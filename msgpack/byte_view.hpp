#ifndef INCLUDED_BYTE_VIEW_HPP
#define INCLUDED_BYTE_VIEW_HPP

#include <array>
#include <bit>
#include <concepts>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <cstddef>
#include <span>
#include <algorithm>
#include <string>

namespace vb {

template <typename T>
concept is_basic_type = std::is_fundamental_v<T>;

template <typename VALUE_TYPE = std::byte>
requires std::same_as<unsigned char, std::make_unsigned_t<VALUE_TYPE>> || std::same_as<std::byte, VALUE_TYPE>
constexpr auto byte_view(const is_basic_type auto& value) {
    return std::bit_cast<std::array<std::byte, sizeof(value)>>(value);
}

template <typename TYPE, std::size_t SIZE, template <typename, std::size_t> typename VIEW>
constexpr TYPE from_bytes(VIEW<std::byte, SIZE> byte_view)
{
    if constexpr (std::same_as<std::string, TYPE>) {
        auto view = std::ranges::transform_view(byte_view, [](std::byte b) { return static_cast<char>(b); });
        return std::string{std::begin(view), std::end(view)};
    } else {
        return std::bit_cast<TYPE>(byte_view);
    }
}

template <typename TYPE, std::ranges::sized_range VIEW>
requires(std::same_as<std::ranges::range_value_t<VIEW>, std::byte> && !std::same_as<std::array<std::byte, sizeof(TYPE)>, VIEW>)
constexpr TYPE from_bytes(const VIEW& data)
{
    if (std::size(data) <= sizeof(TYPE)) {
        std::array<std::byte, sizeof(TYPE)> data_array{};
        std::ranges::copy(data, data_array.begin());
        return from_bytes<TYPE>(data_array);
    } else {
        throw std::runtime_error("Integer won't fit");
    }
}

static_assert(from_bytes<std::string>(std::array{std::byte{65}, std::byte{66}, std::byte{67}}) == "ABC"); // NOLINT(cppcoreguidelines-avoid-magic-numbers)

}

#endif // INCLUDED_BYTE_VIEW_HPP
