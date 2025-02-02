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

namespace details {

template <typename T>
struct remove_all_const {
    using type = std::remove_const_t<T>;
};

template <typename T1, typename T2>
struct remove_all_const<std::pair<T1, T2>>
{
    using type = std::pair<std::remove_const_t<T1>, std::remove_const_t<T2>>;
};

template <typename... Ts>
struct remove_all_const<std::tuple<Ts...>>
{
    using type = std::tuple<std::remove_const_t<Ts>...>;
};

template <typename T>
using remove_all_const_t = remove_all_const<T>::type;

static_assert(std::same_as <
              remove_all_const_t<std::pair<const std::string, int>>,
                                 std::pair<std::string, int>>);
}

template<is_packable TYPE>
constexpr inline auto
unpack(is_packing_source auto source,
       [[maybe_unused]] TYPE& result,
       std::source_location location = std::source_location::current());

template<is_packing_source SOURCE_TYPE, typename TYPE>
constexpr inline auto
unpack_n(const SOURCE_TYPE& source,
         std::unsigned_integral auto count,
         TYPE& result)
{
    auto return_value = std::ranges::subrange(source);
    if constexpr (std::same_as<TYPE, std::string>) {
        auto source_view = source | std::views::take(count) |
                           std::views::transform(
                             [](std::byte c) { return static_cast<char>(c); });
        std::ranges::copy(source_view, std::back_inserter(result));
        return_value = return_value.advance(std::size(result));
    } else if constexpr (is_array_like<TYPE> || is_map_like<TYPE>) {
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
                if constexpr (is_map_like<TYPE>) {
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
              if constexpr (is_array_like<TYPE>) {
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
        if constexpr (std::same_as<TYPE, std::string> || is_array_like<TYPE> || is_map_like<TYPE>) {
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
    } else if (auto count_len = traits.count_length(); count_len > 0) {
        std::size_t count{0};
        return_value = traits.read_count(source, count);
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

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
static_assert([]() {
    std::string value;
    unpack(
      std::array{ std::byte{ 0xa2 }, std::byte{ 65 }, std::byte{ 66 } },
      value);
    return value.size();
}() == 2);
#if 0
static_assert([]() {
    std::size_t value{ 2 };
    unpack(
      std::array{ std::byte{ 0xcd }, std::byte{ 0x00 }, std::byte{ 0x01 } },
      value);
    return value;
}() == 0x100);
static_assert(
 []() {
     std::array<int, 2> value;
     constexpr auto expected = std::array<int, 2>{1, -1};
     unpack(
      std::array{ std::byte{ 0x92 }, std::byte{0x1}, std::byte{0xff} },
      value);
     auto [end1, end2] = std::ranges::mismatch(value, expected);
     return end1 == value.end() && end2 == expected.end();
 }());
#endif
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

}
#endif // INCLUDED_MSGPACK_HPP
