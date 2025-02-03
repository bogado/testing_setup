#ifndef INCLUDED_FORMAT_TYPE_HPP
#define INCLUDED_FORMAT_TYPE_HPP

#include "./types.hpp"

#include <concepts>
#include <cstdint>
#include <ostream>
#include <utility>

namespace vb::msgpack {

enum class category_t : std::uint16_t
{
    BINARY      = 0b0100'0000,
    UNSIGNED    = 0b0010'0000, 
    NUMERIC     = 0b0001'0000,
    STATIC      = 0b0000'1000,
    CONTAINER   = 0b0000'0100,
    FIXED       = 0b0000'0010,
    VALUE       = 0b0000'0001,

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

    constexpr bool operator==(const id_type&) const = default;
    constexpr bool operator!=(const id_type&) const = default;

    friend std::ostream& operator <<(std::ostream& out, id_type id) {
        return out << "ID{" << std::hex << id.value() << "}";
    }
};

struct spec {
    static constexpr category_t category = category_t::UNKNOWN;
    static constexpr std::uint8_t length = 0;
    static constexpr std::uint8_t bit_length = length * 8;

    static constexpr auto acceptable_id_range(id_type format) { return std::pair{format, format}; }
};

template <std::uint8_t BIT_SIZE, category_t CATEGORY>
struct bit_spec : spec {
    static constexpr category_t category = CATEGORY;
    static constexpr std::uint8_t length = BIT_SIZE / 8;
};

template <std::uint8_t BYTE_SIZE, category_t CATEGORY>
struct byte_spec : spec {
    static constexpr category_t category = CATEGORY;
    static constexpr std::uint8_t length = BYTE_SIZE;
};

template <std::uint8_t SPAN, category_t CATEGORY>
struct range_spec : spec {
    static constexpr category_t category = CATEGORY;

    static constexpr std::pair<id_type, id_type> acceptable_id_range(id_type format) { return {format, format + SPAN }; } 
};

template <auto VALUE>
struct value_spec : spec {
    static constexpr auto category = category_t::VALUE | category_t::STATIC;
    static constexpr auto value = VALUE;
};

namespace test { using enum category_t; 
static_assert(bit_spec<16, NO_CATEGORY>::length == byte_spec<2, NO_CATEGORY>::length);
}

template <type_t TYPE, std::derived_from<spec> SPEC, id_type BASE_ID>
struct traits
{
    using enum category_t;
    using enum type_t;
    using spec_type = SPEC;

    static constexpr auto category = spec_type::category;
    static constexpr auto type = TYPE;
    static constexpr auto format = BASE_ID;
    static constexpr auto id_range = spec_type::acceptable_id_range(format);

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
        } else {
            return SPEC::length;
        } 
    }();

    static constexpr auto base_content_size = []() {
        if constexpr (is(CONTAINER)) {
            return 0;
        } else {
            return spec_length;
        }
    }();

    static constexpr auto length_size = []() {
        if constexpr (is(FIXED)) {
            return 0uz;
        } else if constexpr (is(CONTAINER) || is(BINARY)) {
            return spec_length;
        } else {
            return std::false_type{};
        }
    }();

    constexpr auto content_size() const {
        if constexpr (is(FIXED)) {
            return actual.value() - format.value();
        } else if constexpr (is(CONTAINER)) {
            return 0uz;
        } else {
            return spec_length;
        }
    };

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
            } else if constexpr(is(STATIC)) {
                return SPEC::value;
            } else {
                return std::bit_cast<std::int8_t>(actual.value());
            }
        } else {
            return std::false_type{};
        };
    };

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
