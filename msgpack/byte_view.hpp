#ifndef INCLUDED_BYTE_VIEW_HPP
#define INCLUDED_BYTE_VIEW_HPP

#include <array>
#include <bit>
#include <concepts>
#include <type_traits>

namespace vb {

template <typename T>
concept is_basic_type = std::is_fundamental_v<T>;

template <typename VALUE_TYPE = std::byte>
requires std::same_as<unsigned char, std::make_unsigned_t<VALUE_TYPE>> || std::same_as<std::byte, VALUE_TYPE>
auto byte_view(const is_basic_type auto& value) {
    return std::bit_cast<std::array<std::byte, sizeof(value)>>(value);
}

}

#endif // INCLUDED_BYTE_VIEW_HPP
