#ifndef INCLUDED_FORMAT_TYPE_HPP
#define INCLUDED_FORMAT_TYPE_HPP

#include "./types.hpp"

#include <any>
#include <cstdint>
#include <map>
#include <ostream>
#include <utility>
#include <vector>

namespace vb::msgpack {

enum class category_t : std::uint16_t
{
    BITS        = 0b1000'0000,
    SIZED       = 0b0100'0000,
    SIGNED      = 0b0010'0000,
    NUMERIC     = 0b0001'0000,
    VALUE       = 0b0000'1000,
    CONSTANT    = 0b0000'0100,
    CONTAINER   = 0b0000'0010,
    FIXED       = 0b0000'0001,
    NO_CATEGORY = 0b0000'0000,
    UNKNOWN     = 0b1111'1111
};

template <typename T>
concept is_category_or_type = std::same_as<T, category_t> || std::same_as<T, type_t>;

constexpr category_t operator&(
 const category_t a,
 const category_t b)
{
    if (a == category_t::UNKNOWN || b == category_t::UNKNOWN) {
        return category_t::UNKNOWN;
    }

    return category_t{ static_cast<uint8_t>(std::to_underlying(a) bitand
        std::to_underlying(b)) };
}

constexpr category_t operator|(
 const category_t a,
 const category_t b)
{
    if (a == category_t::UNKNOWN || b == category_t::UNKNOWN) {
        return category_t::UNKNOWN;
    }

    return category_t{ static_cast<uint8_t>(std::to_underlying(a) bitor
        std::to_underlying(b)) };
}

constexpr bool is_valid(category_t category)
{
    return category != category_t::NO_CATEGORY
        && category != category_t::UNKNOWN;
}

constexpr bool is_valid(type_t type)
{
    return type != type_t::NO_TYPE;
}

namespace format {

struct id_type {
    std::byte ident;

    constexpr id_type() = default;

    constexpr id_type(std::byte v)
        : ident{v}
    {}

    constexpr id_type(std::uint8_t v)
      : ident{ v }
    {}

    struct reach_t {
        std::uint8_t length;
    };

    constexpr std::uint8_t value() const
    {
        return static_cast<uint8_t>(ident);
    }

    constexpr id_type &operator +=(std::int8_t increment)
    {
        ident = static_cast<std::byte>(value() + increment);
        return *this;
    }

    constexpr id_type operator +(std::int8_t increment) const
    {
        return id_type{static_cast<std::uint8_t>(value() + increment)};
    }

    constexpr id_type operator ++(int)
    {
        id_type other = *this;
        *this += 1;
        return other;
    }

    constexpr id_type &operator ++()
    {
        *this += 1;
        return *this;
    }

    constexpr id_type operator++() const {
        return *this + 1;
    }

    constexpr id_type middle(id_type other) const {
        return id_type{static_cast<std::byte>(value() + other.value()/2)};
    }

    constexpr bool inside(std::pair<id_type, id_type> range) {
        return value() >= range.first.value() &&
               value() <= range.second.value();
    }

    template<typename VALUE_T>
        requires(sizeof(VALUE_T) == 1)
    struct directValue
    {
        using value_type = VALUE_T;
        value_type value;
    };

    template<typename VALUE_T>
    requires (sizeof(VALUE_T) == 1) 
    constexpr id_type(id_type predecessor, directValue<VALUE_T> val, VALUE_T value) :
        ident{predecessor.value() + value -
            val.value()}
    {}

    constexpr bool operator==(const id&) const = default;
    constexpr bool operator!=(const id&) const = default;
};

template <type_t TYPE, category_t CATEGORY, std::uint8_t ID, std::int8_t SPEC>
struct traits
{
    using enum category_t;
    using enum type_t;

    static constexpr category_t category = CATEGORY;
    static constexpr type_t  type = TYPE;
    static constexpr id_type format = id_type{ID};

    id_type actual;

    constexpr static bool is(type_t type_b)
    {
        return type_b == type;
    }

    constexpr static bool is(category_t FMT_TYPE)
    {
        return is_valid(FMT_TYPE & category);
    }

    static constexpr bool accepts(id_type other) 
    {
        return other.inside(id_range);
    }

    static constexpr auto spec_length = []() {
        if constexpr (is(VALUE) || is(FIXED)) {
            return 0;
        } else if constexpr (is(BITS)) {
            return SPEC / 8;
        } else {
            return SPEC;
        } 
    }();

    static constexpr auto base_content_size = []() {
        if constexpr (is(CONTAINER)) {
            return 0;
        } else {
            return spec_length;
        }
    }();

    constexpr auto content_size() const {
        if constexpr (is(FIXED)) {
            if constexpr (is(CONTAINER)) {
                return base_content_size + actual.value() - format.value();
            } else {
                return spec_length;
            }
        } else {
            return std::false_type{};
        }
    };

    using standard_type =
        std::conditional_t<
            is(INTEGER), integer<base_content_size, is(SIGNED)>,
        std::conditional_t<
            is(FLOAT), floating<bit_size<SPEC>>,
        std::conditional_t<
            is(BOOL), bool,
        std::conditional_t<
            is(STR), std::string,
        std::conditional_t<
            is(ARRAY), std::vector<std::any>,
        std::conditional_t<
            is(MAP), std::map<std::any, std::any>,
        std::conditional_t< 
            is(EXT), ext<bit_size<SPEC>>,
        std::conditional_t<
            is(BIN), std::vector<std::byte>,
        std::conditional<
            is(VOID), std::nullptr_t,
        std::false_type >>>>>>>>>;

    template<is_packable T>
    static constexpr bool accepts_type =
      (is(STR) && std::same_as<T, std::string>) ||
      (is(INTEGER) && std::is_integral_v<T>) ||
      (is(FLOAT) && std::is_floating_point_v<T>) ||
      (is(BOOL) && std::same_as<T, bool>) || (is(ARRAY) && is_array_like<T>) ||
      (is(MAP) && is_map_like<T>) || (is(EXT) && is_ext_like<T>) ||
      (is(BIN) && is_buffer_like<T>) || (is(VOID) && std::is_null_pointer_v<T>);

    constexpr auto value() {
        if constexpr (is(VALUE)) {
            if constexpr(is(VOID)) {
                return nullptr;
            } else if constexpr(is(CONSTANT)) {
                return static_cast<standard_type>(SPEC);
            } else {
                return std::bit_cast<std::int8_t>(actual.value());
            }
        } else {
            return std::false_type{};
        };
    };

    static constexpr auto id_range = []() {
        if constexpr (is(VALUE) && !is(CONSTANT)) {
            id_type first = format;
            id_type second = format + SPEC;
                return std::pair{ id_type{ first }, id_type{ second } };
        } else if constexpr (is(FIXED)) {
            return std::pair { format, format + SPEC};
        } else {
            return std::pair{ format, format }; 
        };
    }();

    static constexpr bool belongs(id_type val) {
        auto range = id_range();
        if constexpr (std::same_as<decltype(range), std::false_type>) {
            return false;
        }
        return val.value() >= range.first && val.value() <= range.second;
    }
};

}
}
#endif // 
