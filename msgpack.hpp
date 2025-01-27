#ifndef INCLUDED_MSGPACK_HPP
#define INCLUDED_MSGPACK_HPP

#include "./msgpack/format.hpp"
#include "msgpack/byte_view.hpp"
#include "msgpack/types.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <format>
#include <iterator>
#include <ranges>
#include <source_location>
#include <stdexcept>
#include <type_traits>

namespace vb::msgpack {

template <is_packable TYPE>
constexpr auto unpack(is_packing_source auto source, [[maybe_unused]] TYPE& result, std::source_location location = std::source_location::current()) 
{
    using namespace format;
    using std::ranges::subrange;

    constexpr auto unpack_n = []<is_packing_source SOURCE_TYPE>(const SOURCE_TYPE& source,
                                 std::unsigned_integral auto count,
                                 TYPE& result) {
        auto return_value = std::ranges::subrange(source);
        if constexpr (std::same_as<TYPE, std::string>) {
            auto source_view = source | std::views::take(count) |
                               std::views::transform([](std::byte c) {
                                   return static_cast<char>(c);
                               });
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

            using value_type = std::ranges::range_value_t<TYPE>;
            auto output = [&]() {
                if constexpr (requires { result.push_back(value_type{}); }) {
                    return std::back_inserter(result);
                } else if constexpr (requires {
                                         result.insert(value_type{});
                                     }) {
                    return std::inserter(result, std::begin(result));
                } else if constexpr (std::tuple_size_v<TYPE> > 0) {
                    return std::begin(result);
                }
            }();

            std::ranges::generate_n(output, count, [&return_value]() {
                value_type next{};
                if constexpr (is_array_like<TYPE>) {
                    return_value = unpack(return_value, next);
                } else {
                    return_value = unpack(return_value, next.first);
                    return_value = unpack(return_value, next.second);
                }
                    return next;
            });
        }
        return return_value;
    };

    auto return_value = subrange(source);
    auto traits = classification{ source.front() };
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
    } else if (traits.count_length() > 0) {
        std::size_t count{ 0 };
        return_value = traits.read_count(source, count);
        if constexpr (std::same_as<TYPE, std::string> || is_array_like<TYPE> ||
                      is_map_like<TYPE>) {
            return_value = unpack_n(return_value, count, result);
        } else {
            auto data = std::array<std::byte, sizeof(TYPE)>{};
            std::ranges::copy(data | std::views::take(count), data.begin());
            result = from_bytes<TYPE>(data);
        }
    } else if (traits.content_size() > 0) {
        return_value = unpack_n(return_value, traits.content_size(), result);
        return_value = return_value.advance(traits.content_size());
    }
    if constexpr (std::is_arithmetic_v<TYPE>) {
        if (traits.content_size() <= sizeof(TYPE)) {
            result = from_bytes<TYPE>(return_value);
            return_value = return_value.advance(sizeof(TYPE));
        } else {
            throw "not implemented yet";
        }
    }
    return return_value;
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
static_assert([]() {
    std::string value;
    unpack(
      std::array{ std::byte{ 0xa2 }, std::byte{ 65 }, std::byte{ 66 } },
      value);
    return value.size();
}() == 2);
static_assert([]() {
    std::size_t value{ 2 };
    unpack(
      std::array{ std::byte{ 0xcd }, std::byte{ 0x00 }, std::byte{ 0x01 } },
      value);
    return value;
}() == 0x100);
#if 0
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
