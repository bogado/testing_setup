#ifndef INCLUDED_BYTE_VIEW_HPP
#define INCLUDED_BYTE_VIEW_HPP

#include "format_type.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace vb {

template<typename T>
concept is_basic_type = std::is_fundamental_v<T>;

template<std::endian ENDIAN = std::endian::native, std::size_t SIZE>
constexpr auto endianess_adjust(std::array<std::byte, SIZE> data) {
    if constexpr (ENDIAN == std::endian::native) {
        return data;
    } else {
        auto result = data;
        std::ranges::reverse_copy(data, result.begin());
        return result;
    }
}

template<std::endian ENDIAN = std::endian::native>
constexpr auto
byte_view(const is_basic_type auto& value)
{
    return endianess_adjust<ENDIAN>(std::bit_cast<std::array<std::byte, sizeof(value)>>(value));
}

template<typename TYPE, std::size_t SIZE>
    requires(std::same_as<std::string, TYPE>)
constexpr TYPE
from_bytes(std::span<std::byte, SIZE> byte_view)
{
    TYPE result{};
    std::ranges::copy(
     std::ranges::transform_view{
         byte_view, [](auto val) { return static_cast<char>(val); }},
         std::back_inserter(result));
    return result;
}

template <typename TYPE, std::size_t SIZE>
requires(std::is_fundamental_v<TYPE> && SIZE == sizeof(TYPE))
constexpr TYPE
from_bytes(std::span<std::byte, sizeof(TYPE)> byte_view)
{
    std::array<std::byte, sizeof(TYPE)> source;
    std::ranges::copy(byte_view, std::begin(source));
    return std::bit_cast<TYPE>(source); 
}

template<typename TYPE, std::size_t SIZE>
requires(std::ranges::range<TYPE> &&
 sizeof(std::ranges::range_value_t<TYPE>) > 1 && SIZE %
 sizeof(std::ranges::range_value_t<TYPE>) == 0)
constexpr TYPE from_bytes(std::span<std::byte, SIZE> data)
{
    static constexpr auto type_size = sizeof(TYPE);
    TYPE result{};
    result.reserve(SIZE/type_size);

    std::ranges::copy(data | std::views::chunk(type_size) | std::views::transform([](auto& view) {
        return from_bytes<std::ranges::range_value_t<TYPE>>(std::span<std::byte, type_size>{std::begin(view), std::end(view)});
    }), std::back_insert_iterator{result});
    return result;   
}

template <
    typename TYPE,
    std::endian ENDIAN = std::endian::native,
    std::size_t SIZE>
constexpr TYPE from_bytes(std::array<std::byte, SIZE> data)
{
    auto adjusted = endianess_adjust<ENDIAN>(data);
    return from_bytes<TYPE, SIZE>(std::span<std::byte, SIZE>{adjusted});
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
static_assert(from_bytes<std::string>(std::array{ std::byte{ 65 },
                                                  std::byte{ 66 },
                                                  std::byte{ 67 } }) ==
              "ABC"); 
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}

#endif // INCLUDED_BYTE_VIEW_HPP
