#ifndef INCLUDED_MSGPACK_HPP
#define INCLUDED_MSGPACK_HPP

#include "./msgpack/format.hpp"
#include "msgpack/byte_view.hpp"
#include "msgpack/types.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <format>
#include <iterator>
#include <numeric>
#include <ranges>
#include <source_location>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace vb::msgpack {


template<is_packable TYPE>
constexpr inline auto
unpack(is_packing_source auto source,
       [[maybe_unused]] TYPE& result,
       std::source_location location = std::source_location::current());

template<is_packing_source SOURCE_TYPE, is_packable TYPE>
constexpr inline auto
unpack_n(const SOURCE_TYPE& source,
         std::unsigned_integral auto count,
         TYPE& result)
{
    using enum type_t;
    auto return_value = std::ranges::subrange(source);
    if constexpr (std::same_as<TYPE, std::string>) {
        auto source_view = source | std::views::take(count) |
                           std::views::transform(
                             [](std::byte c) { return static_cast<char>(c); });
        std::ranges::copy(source_view, std::back_inserter(result));
        return_value = return_value.advance(std::size(result));
    } else if constexpr (constexpr auto type = type_of<TYPE>; type == ARRAY || type == MAP) {
        if constexpr (requires {
                          { result.clear() };
                      }) {
            result.clear();
            if constexpr (requires {
                              { result.reserve(count) };
                          }) {
                result.reserve(count);
            }
        }

        using value_type =
          details::remove_all_const_t<std::ranges::range_value_t<TYPE>>;
        auto output
          [[maybe_unused]] =
            [&](std::integral auto count [[maybe_unused]]) {
                if constexpr (type_accepts<MAP, TYPE>) {
                    return std::inserter(result, std::end(result));
                } else if constexpr (requires { result.push_back(value_type{}); }) {
                    return std::back_inserter(result);
                } else if constexpr (std::ranges::sized_range<TYPE>) {
                    if (std::size(result) < count) {
                        throw std::logic_error("Size of container does not support " + std::to_string(count) + " elements at " + std::source_location::current().function_name());
                    }
                    return std::begin(result);
                }
                throw std::logic_error{std::string{"cannot form a output iterator at "} + std::source_location::current().function_name()};
            };

        std::ranges::generate_n(
          output(count), count, [&return_value]() -> value_type {
              value_type next{};
              if constexpr (type_accepts<ARRAY, TYPE>) {
                  return_value = unpack(return_value, next);
              } else {
                  auto& [key, value] = next;
                  return_value = unpack(return_value, key);
                  return_value = unpack(return_value, value);
              }
              return next;
          });
    }
    return return_value;
};

template <is_packable TYPE>
constexpr inline auto unpack(is_packing_source auto source, [[maybe_unused]] TYPE& result, std::source_location location) 
{
    using namespace format;
    using std::ranges::subrange;
    using enum type_t;

    auto return_value = subrange(source);
    auto traits = classification{ return_value.front() };

    if (!traits.accepts<TYPE>()) {
        std::string error = std::format("{}:{} at {} : type is not acceptable by the format {:x} ({:x})", location.file_name(),location.line(), std::source_location::current().function_name(), traits.format_id().value(), traits.main_format_id().value());
        throw std::domain_error(error);
    }

    return_value = return_value.advance(1);
    if (traits.is_value()) {
        if constexpr (std::is_integral_v<TYPE>) {
            result = static_cast<TYPE>(traits.value().value_or(0));
        } else {
            throw std::logic_error(
             "is_value is not compatible with this type");
        }
    } else if (auto content_size = traits.content_size(); content_size > 0) {
        if constexpr (std::same_as<TYPE, std::string> || type_accepts<ARRAY, TYPE> || type_accepts<MAP, TYPE>) {
            return_value = unpack_n(return_value, content_size, result);
        } else if (traits.is(type_t::INTEGER)) {
            if constexpr(std::integral<TYPE>) {
            auto partial_source = subrange(return_value,content_size) | std::views::transform([](auto byte) { return static_cast<uint8_t>(byte); });
            result = std::accumulate(std::begin(partial_source), std::end(partial_source), TYPE{0}, [](TYPE value, auto next) {
                return (value << 8) + static_cast<std::uint8_t>(next);
            });
            return_value = return_value.advance(content_size);
            } else {
                throw std::logic_error("Invalid type");
            }
        } else {
            auto data = std::array<std::byte, sizeof(TYPE)>{};
            std::ranges::copy(return_value | std::views::take(content_size), data.begin());
            result = from_bytes<TYPE, std::endian::big>(data);
            return_value = return_value.advance(content_size);
        }
    } else if (auto count_len = traits.length_size(); count_len > 0) {
        std::size_t count{0};
        return_value = traits.read_count(return_value, count);
            return_value = unpack_n(return_value, content_size, result);
        unpack_n(return_value, count, result);
    } else if (traits.content_size() <= sizeof(TYPE)) {
        if constexpr (std::is_arithmetic_v<TYPE>) {
            std::array<std::byte, sizeof(TYPE)> data{};
            std::ranges::copy(return_value | std::views::take(sizeof(TYPE)), std::begin(data));
            result = from_bytes<TYPE, std::endian::big>(data);
            return_value = return_value.advance(sizeof(TYPE));
        } else {
            throw "not implemented yet";
        }
    }
    return return_value;
}

template <typename TYPE, std::size_t SIZE>
TYPE from_bytes(std::span<std::byte, SIZE> source) {
    std::array<std::byte, SIZE> data;
    return from_bytes<TYPE>(std::ranges::copy(source, std::begin(data)));
}

namespace test {
template <typename TYPE, std::uint8_t... DATA>
constexpr TYPE unpack_data() {
    TYPE result{};
    unpack(std::array{std::byte(DATA)...}, result);
    return result;
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
static_assert(unpack_data<std::string, 0xa2,  'a',  'b'>() == "ab");
static_assert(unpack_data<std::string, 0xd9, 2,  'a',  'b'>() == "ab");
static_assert(unpack_data<std::string, 0xda, 0, 2,  'a',  'b'>() == "ab");
static_assert(unpack_data<int, 0xcd ,  0x00 ,  0x01 >() == 0x1);
static constexpr auto expected = std::array{1, -1};
static constexpr auto obtained = unpack_data<std::array<int, 2>, 0x92 , 0x1, 0xff>();
static_assert(expected[0] == obtained[0]);
static_assert(expected[1] == obtained[1]);
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}
}
#endif // INCLUDED_MSGPACK_HPP
