#ifndef INCLUDED_BYTE_VIEW_HPP
#define INCLUDED_BYTE_VIEW_HPP

#include <array>
#include <bit>
#include <concepts>
#include <ranges>
#include <type_traits>

namespace vb {

template <typename T>
concept is_basic_type = std::is_fundamental_v<T>;

template <typename VALUE_TYPE = std::byte>
requires std::same_as<unsigned char, std::make_unsigned_t<VALUE_TYPE>> || std::same_as<std::byte, VALUE_TYPE>
constexpr auto byte_view(const is_basic_type auto& value) {
    return std::bit_cast<std::array<std::byte, sizeof(value)>>(value);
}

template <is_basic_type TYPE, std::ranges::sized_range DATA_VIEW>
requires(sizeof(std::ranges::range_value_t<DATA_VIEW>) == 1)
TYPE from_bytes(const DATA_VIEW& byte_view)
{
    return std::bit_cast<TYPE>(byte_view);
}

#endif // INCLUDED_BYTE_VIEW_HPP
