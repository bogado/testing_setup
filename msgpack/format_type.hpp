#ifndef INCLUDED_FORMAT_TYPE_HPP
#define INCLUDED_FORMAT_TYPE_HPP

#include "./types.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>
#include <concepts>

namespace vb::msgpack {


struct format_type {

enum class type_t : std::uint8_t
{
    INTEGER,
    BOOL,
    FLOAT,
    STR,
    ARRAY,
    MAP,
    EXT,
    VOID,
    BIN
};
using enum type_t;

enum class category_t: std::uint8_t
{
    UNKNOWN      = 0b100'0000,
    SIGNED       = 0b010'0000,
    NUMERIC      = 0b001'0000,
    VALUE        = 0b000'1000,
    CONTAINER    = 0b000'0100,
    FIXED        = 0b000'0010,
    DIRECT       = 0b000'0001,
    NO_CATEGORY  = 0b000'0000
};
using enum category_t;

type_t type;
category_t category;
std::byte min_id;
std::int8_t id_range;

constexpr bool operator==(const format_type&) const = default;
constexpr bool operator==(const type_t& other) const {
    return type == other;
}


constexpr bool has_length() const;
};

constexpr format_type::category_t operator &(const format_type::category_t a, const format_type::category_t b) {
    return format_type::category_t{std::to_underlying(a) and std::to_underlying(b)};    
}

constexpr format_type::category_t operator |(const format_type::category_t a, const format_type::category_t b) {
    return format_type::category_t{std::to_underlying(a) or std::to_underlying(b)};    
}

constexpr bool operator==(const format_type& format, const format_type::category_t& other) {
    return (format.category | other) != format_type::category_t::NO_CATEGORY;
}

constexpr bool format_type::has_length() const{
    return (category & (FIXED | DIRECT)) != NO_CATEGORY;
}

template<typename TYPE, format_type FMT_TYPE>
concept is =
  ((FMT_TYPE == format_type::STR && is_str_like<TYPE>) ||
   (FMT_TYPE == format_type::MAP && is_map_like<TYPE>) ||
   (FMT_TYPE == format_type::CONTAINER && std::ranges::sized_range<TYPE>) ||
   (FMT_TYPE == format_type::ARRAY && is_array_like<TYPE>) ||
   (FMT_TYPE == format_type::NUMERIC && std::is_arithmetic_v<TYPE>) ||
   (FMT_TYPE == format_type::FLOAT && std::is_floating_point_v<TYPE>) ||
   (FMT_TYPE == format_type::INTEGER && std::integral<TYPE>) ||
   (FMT_TYPE == format_type::SIGNED && std::is_signed_v<TYPE>) ||
   (FMT_TYPE == format_type::BOOL && std::same_as<TYPE, bool>));

template <format_type FMT_TYPE, is<FMT_TYPE> TYPE>
std::uintmax_t measure(const TYPE& object) {
        if constexpr (FMT_TYPE == format_type::CONTAINER) {
            return object.size();
        } else {
            return sizeof(object);
        }
    }
}

#endif // INCLUDED_FORMAT_TYPE_HPP

