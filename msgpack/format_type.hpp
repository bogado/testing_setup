#ifndef INCLUDED_FORMAT_TYPE_HPP
#define INCLUDED_FORMAT_TYPE_HPP

#include "./types.hpp"
#include "./format_id.hpp"

#include <concepts>
#include <cstdint>
#include <utility>

namespace vb::msgpack::format {

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
    if (a == category_t::NO_CATEGORY) { return b; }
    if (b == category_t::NO_CATEGORY) { return a; }

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
    if (a == category_t::NO_CATEGORY) { return b; }
    if (b == category_t::NO_CATEGORY) { return a; }

    return category_t{static_cast<uint8_t>(std::to_underlying(a) bitor
        std::to_underlying(b))};
}

constexpr bool is_valid(category_t category)
{
    return category != category_t::NO_CATEGORY
        && category != category_t::UNKNOWN;
}

template <typename SPEC>
concept is_spec = requires {
    { SPEC::category } -> std::convertible_to<category_t>;
    { SPEC::length } -> std::convertible_to<std::size_t>;
    { SPEC::format } -> std::same_as<const id_type&>;
    { SPEC::id_range } -> std::same_as<const std::pair<id_type, id_type>&>;
};

template <id_type ID, std::size_t LENGTH, category_t CATEGORY, std::uint8_t MAX_RANGE = 0>
struct spec {
   static constexpr category_t category = CATEGORY;
   static constexpr std::uint8_t length = LENGTH;
   static constexpr auto format = ID;
   static constexpr auto id_range = std::pair<id_type, id_type>{ format, format + MAX_RANGE};
};

template <id_type ID, std::uint8_t BIT_SIZE, category_t CATEGORY>
requires (BIT_SIZE % 8 == 0)
using bit_spec = spec<ID, BIT_SIZE/8, CATEGORY>;

template <id_type ID, std::uint8_t SPAN, category_t CATEGORY>
using range_spec = spec<ID, 0, CATEGORY, SPAN>;

template <id_type ID, std::uint8_t SPAN, category_t CATEGORY>
using fixed_size_container = spec<ID, 0, CATEGORY, SPAN>;

template <id_type ID>
using nil_spec = spec<ID, 0, category_t::NO_CATEGORY>;

template <id_type ID, auto VALUE>
struct value_spec : spec<ID, 0, category_t::VALUE> {
    static constexpr auto value = VALUE;
};

template <type_t TYPE, is_spec SPEC>
struct traits
{
    using enum category_t;
    using enum type_t;
    using spec_type = SPEC;

    static constexpr auto category = spec_type::category;
    static constexpr auto type = TYPE;
    static constexpr auto format = spec_type::format;
    static constexpr auto id_range = spec_type::id_range;

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
        return SPEC::length;
    }();

    static constexpr auto base_content_size = []() {
        return spec_length;
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

    template<typename T>
    static constexpr bool accepts_type = type_accepts<type, T>;

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

template <typename TRAITS>
concept is_traits = std::same_as<TRAITS, traits<TRAITS::type,typename TRAITS::spec_type>>;

}
#endif // 
