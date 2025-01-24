#ifndef INCLUDED_FORMAT_TYPE_HPP
#define INCLUDED_FORMAT_TYPE_HPP

#include "./types.hpp"

#include <any>
#include <cstdint>
#include <map>
#include <utility>

namespace vb::msgpack {
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
    BIN,
    NO_TYPE
};

enum class category_t : std::uint8_t
{
    UNKNOWN     = 0b100'0000,
    SIZED       = 0b010'0000,
    SIGNED      = 0b001'0000,
    NUMERIC     = 0b000'1000,
    VALUE       = 0b000'0100,
    CONTAINER   = 0b000'0010,
    FIXED       = 0b000'0001,
    NO_CATEGORY = 0b000'0000
};

constexpr category_t operator&(
 const category_t a,
 const category_t b)
{
    return category_t{ static_cast<uint8_t>(std::to_underlying(a) bitand
        std::to_underlying(b)) };
}

constexpr category_t operator|(
 const category_t a,
 const category_t b)
{
    return category_t{ static_cast<uint8_t>(std::to_underlying(a) bitor
        std::to_underlying(b)) };
}

constexpr bool is_valid(category_t category)
{
    return category != category_t::NO_CATEGORY;
}

constexpr bool is_valid(type_t type)
{
    return type != type_t::NO_TYPE;
}

namespace format {

struct id {
    std::byte ident;

    constexpr id(std::byte v)
        : ident{v}
    {}

    constexpr id(std::uint8_t v)
      : ident{ v }
    {}

    struct reach_t {
        std::uint8_t length;
    };

    constexpr const id operator ++(int)
    {
        auto other = *this;
        ++(*this);
        return other;
    }

    constexpr id &operator ++()
    {
        ident = static_cast<std::byte>(std::to_underlying(ident)+1);
        return *this;
    }

    constexpr id operator++() const {
        auto other = *this;
        return ++other;
    }

    constexpr id middle(id other) const {
        return id{static_cast<std::byte>(value() + other.value()/2)};
    }

    constexpr std::uint8_t value() const
    {
        return static_cast<uint8_t>(ident);
    }

    template<typename VALUE_T>
    requires (sizeof(VALUE_T) == 1) 
    struct directValue {
        using value_type = VALUE_T;
        value_type value;
    };

    template<typename VALUE_T>
    requires (sizeof(VALUE_T) == 1) 
    constexpr id(id predecessor, directValue<VALUE_T> val, VALUE_T value) :
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
    static constexpr type_t type = TYPE;
    static constexpr id format = id{ID};

    constexpr static bool is(type_t type_b)
    {
        return type_b == type;
    }

    constexpr static bool is(category_t FMT_TYPE)
    {
        return is_valid(FMT_TYPE & category);
    }

    static constexpr bool accepts(id other) 
    {
        if constexpr (is(VALUE) && is(NUMERIC)) {
            auto [min, max] = value_range;
            return other.value() >= min && other.value() <= max;
        } else {
           return other == format;
        }
    }

    using standard_type =
        std::conditional_t<
            is(INTEGER) && !is(VALUE), integer<SPEC, is(SIGNED)>,
        std::conditional_t<
            is(INTEGER) && is(VALUE), integer<8, is(SIGNED)>,
        std::conditional_t<
            is(BOOL), bool,
        std::conditional_t<
            is(FLOAT), floating<bit_size<SPEC>>,
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
        std::false_type >>>>>>>>>>;

    template<is_packable T>
    static constexpr bool accepts_type =
      (is(INTEGER) && std::is_integral_v<T>) ||
      (is(FLOAT) && std::is_floating_point_v<T>) ||
      (is(BOOL) && std::same_as<T, bool>) || (is(ARRAY) && is_array_like<T>) ||
      (is(MAP) && is_map_like<T>) || (is(EXT) && is_ext_like<T>) ||
      (is(BIN) && is_buffer_like<T>) || (is(VOID) && std::is_null_pointer_v<T>);

    static constexpr auto value = []() {
        if constexpr (is(VALUE)) {
            if constexpr(is(BOOL)) {
                return SPEC == 1;
            } else if constexpr(is(VOID)) {
                return nullptr;
            } else {
                return static_cast<standard_type>(format.value());
            }
        } else {
            return std::false_type{};
        };
    }();

    static constexpr auto value_range = []() {
        if constexpr (is(VALUE)) {
            if constexpr(is(BOOL) || is(VOID)) {
                return std::pair{value, value};
            } else {
                auto first = standard_type{value};
                auto second = static_cast<standard_type>(first + standard_type{SPEC});
                if (first < second) {
                    return std::pair{first, second};
                } else {
                    return std::pair{second, first};
                }
            }
        } else {
            return std::false_type{};
        };
    }();

    static constexpr bool belongs(id val) {
        auto range = value_range();
        if constexpr (std::same_as<decltype(range), std::false_type>) {
            return false;
        }
        return val.value() >= range.first && val.value() <= range.second;
    }

    static constexpr auto count_bytes = []() {
        if constexpr (is(CONTAINER)) {
            return SPEC / 8;
        } else if constexpr (is(SIZED)) {
            return SPEC;
        } else if constexpr (is(VALUE) || is(FIXED)) {
            return 0;
        } else {
            return std::false_type{};
        }
    }();

    static constexpr auto content_size = []() {
        if constexpr (is(FIXED)) {
            if constexpr (is(NUMERIC)) {
                return SPEC/8;
            } else {
                return SPEC;
            }
        } else {
            return std::false_type{};
        }
    }();

    template <is_packing_source SOURCE_T, std::integral OUT_TYPE>
    constexpr auto read_count(is_packing_source auto source, std::integral auto size, OUT_TYPE& count)
    {
        count = 0;
        for (auto value: source | std::views::take(size) |
         std::views::transform([](std::byte value) { return
             static_cast<uint8_t>(value); })) {
            count += value;
            count <<= 1;
        }
        return std::ranges::subrange(source.begin() + size, source.end());
    }

    template <is_packing_source SOURCE_T, is_packable OUT_TYPE>
    constexpr auto read_data(is_packing_source auto source, OUT_TYPE& out_data)
    {
    }

    template <is_packing_source SOURCE_T, is_packable OUT_TYPE>
    constexpr auto read_data(is_packing_source auto source, std::integral auto count, OUT_TYPE& out_data)
    {
    }
};

}}
#endif // 
