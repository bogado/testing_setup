#ifndef INCLUDED_FORMAT_HPP
#define INCLUDED_FORMAT_HPP

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <utility>

#include "./types.hpp"
#include "./format_type.hpp"

namespace vb::msgpack {

struct format {
    using enum format_type::type_t;
    using enum format_type::category_t;

    static constexpr auto POSITIVE_FIX_INT = format_type{ INTEGER, DIRECT,            std::byte{0}, 0x7f};
    static constexpr auto FIX_MAP = format_type{          MAP,     CONTAINER | FIXED, std::byte{0x80}, 0xf};
    static constexpr auto FIX_ARRAY = format_type{        ARRAY,   CONTAINER | FIXED, std::byte{0x90}, 0xf};
    static constexpr auto FIX_STR = format_type{          STR,     CONTAINER | FIXED, std::byte{0xa0}, 0x1f};
    static constexpr auto NIL = format_type{              VOID,    DIRECT,            std::byte{0xc0}, 1};
    static constexpr auto UNUSED = format_type{           VOID,    UNKNOWN,           std::byte{0xc1}, 1};
    static constexpr auto TRUE = format_type{             BOOL,    DIRECT,            std::byte{0xc2}, 1};
    static constexpr auto FALSE = format_type{            BOOL,    DIRECT,            std::byte{0xc3}, 1};
    static constexpr auto BIN_8 = format_type{            BIN,     CONTAINER,         std::byte{0xc4}, 1};
    static constexpr auto BIN_16 = format_type{           BIN,     CONTAINER,         std::byte{0xc5}, 1};
    static constexpr auto BIN_32 = format_type{           BIN,     CONTAINER,         std::byte{0xc6}, 1};
    static constexpr auto EXT_8 = format_type{            EXT,     CONTAINER,         std::byte{0xc7}, 1};
    static constexpr auto EXT_16 = format_type{           EXT,     CONTAINER,         std::byte{0xc8}, 1};
    static constexpr auto EXT_32 = format_type{           EXT,     CONTAINER,         std::byte{0xc9}, 1};
    static constexpr auto FLOAT_32 = format_type{         EXT,     CONTAINER,         std::byte{0xca}, 1};
    static constexpr auto FLOAT_64 = format_type{         EXT,     CONTAINER,         std::byte{0xcb}, 1};
    static constexpr auto UINT_8 = format_type{           INTEGER, FIXED,             std::byte{0xcc}, 1};
    static constexpr auto UINT_16 = format_type{          INTEGER, FIXED,             std::byte{0xcd}, 1};
    static constexpr auto UINT_32 = format_type{          INTEGER, FIXED,             std::byte{0xce}, 1};
    static constexpr auto UINT_64 = format_type{          INTEGER, FIXED,             std::byte{0xcf}, 1};
    static constexpr auto INT_8 = format_type{            INTEGER, SIGNED | FIXED,    std::byte{0xd0}, 1};
    static constexpr auto INT_16 = format_type{           INTEGER, SIGNED | FIXED,    std::byte{0xd1}, 1};
    static constexpr auto INT_32 = format_type{           INTEGER, SIGNED | FIXED,    std::byte{0xd2}, 1};
    static constexpr auto INT_64 = format_type{           INTEGER, SIGNED | FIXED,    std::byte{0xd3}, 1};
    static constexpr auto FIXEXT_1 = format_type{         EXT,     FIXED,             std::byte{0xd4}, 1};
    static constexpr auto FIXEXT_2 = format_type{         EXT,     FIXED,             std::byte{0xd5}, 1};
    static constexpr auto FIXEXT_4 = format_type{         EXT,     FIXED,             std::byte{0xd6}, 1};
    static constexpr auto FIXEXT_8 = format_type{         EXT,     FIXED,             std::byte{0xd7}, 1};
    static constexpr auto FIXEXT_16 = format_type{        EXT,     FIXED,             std::byte{0xd8}, 1};
    static constexpr auto STR_8 = format_type{            STR,     FIXED,             std::byte{0xd9}, 1};
    static constexpr auto STR_16 = format_type{           STR,     FIXED,             std::byte{0xda}, 1};
    static constexpr auto STR_32 = format_type{           STR,     FIXED,             std::byte{0xdb}, 1};
    static constexpr auto ARRAY_16 = format_type{         ARRAY,   FIXED,             std::byte{0xdc}, 1};
    static constexpr auto ARRAY_32 = format_type{         ARRAY,   FIXED,             std::byte{0xdd}, 1};
    static constexpr auto MAP_16 = format_type{           MAP,     FIXED,             std::byte{0xde}, 1};
    static constexpr auto MAP_32 = format_type{           MAP,     FIXED,             std::byte{0xdf}, 1};
    static constexpr auto NEGATIVE_FIX_INT = format_type{ INTEGER, DIRECT | SIGNED,   std::byte{0xe0}, -0x1f};

    format_type details;

    bool operator==(const format_type& other) const
    {
        return details == other; 
    }
};

template <format FORMAT>
struct format_traits
{
    static constexpr format value = FORMAT;

private:
    static constexpr auto nil_like(std::byte value) -> values_t {
        return values_t{0, std::byte{0}, value, 0, format::UNUSED};
    };

    static constexpr auto variable_format_id(std::uint8_t len, int start, format next) -> values_t {
        return values_t{0, static_cast<std::byte>(start), static_cast<std::byte>(start+len), 0, next};
    };

    static constexpr auto byte_count(std::integral auto count, format next) -> values_t {
        return values_t{ static_cast<uint8_t>(count), std::byte{0}, std::byte{0}, std::int8_t{0}, next };
    };

    static constexpr auto fixed(std::int8_t maximum, format next) -> values_t {
        return { 0, std::byte{0}, std::byte{0}, static_cast<std::int8_t>(maximum), next };
    };

    constexpr static auto values = []()
    {
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
        static constexpr auto vals =
          std::array{ variable_format_id(0x7f, 0, format::INT_8),
                      variable_format_id(0x0f, 0x80, format::MAP_16),
                      variable_format_id(0x0f, 0x90, format::ARRAY_16),
                      variable_format_id(0x1f, 0xa0, format::STR_8),
                      nil_like(std::byte{ 0xc0 }),
                      nil_like(std::byte{ 0xc1 }),
                      variable_format_id(2, 0xc2, format::UNUSED),
                      byte_count(1, format::BIN_16),
                      byte_count(2, format::BIN_32),
                      byte_count(4, format::UNUSED),
                      byte_count(1, format::EXT_16),
                      byte_count(2, format::EXT_32),
                      byte_count(4, format::UNUSED),
                      fixed(4, format::FLOAT_32),
                      fixed(8, format::UNUSED),
                      fixed(0, format::UINT_16),
                      fixed(1, format::UINT_32),
                      fixed(2, format::UINT_64),
                      fixed(4, format::UNUSED),
                      fixed(0, format::INT_16),
                      fixed(1, format::INT_32),
                      fixed(2, format::INT_64),
                      fixed(4, format::UNUSED),
                      fixed(1, format::FIXEXT_2),
                      fixed(2, format::FIXEXT_4),
                      fixed(4, format::FIXEXT_8),
                      fixed(8, format::FIXEXT_16),
                      fixed(16, format::UNUSED),
                      byte_count(1, format::STR_16),
                      byte_count(2, format::STR_32),
                      byte_count(4, format::UNUSED),
                      byte_count(2, format::ARRAY_32),
                      byte_count(4, format::UNUSED),
                      byte_count(2, format::MAP_32),
                      byte_count(4, format::UNUSED),
                      variable_format_id(0x1f, -1, format::POSITIVE_FIX_INT) };
        return vals[std::to_underlying(FORMAT)];
    }();
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
public:
    static constexpr std::uint8_t size_len = std::get<std::uint8_t>(values); // Number of bytes the size takes.
    static constexpr std::byte minimum_format_id = std::get<1>(values);
    static constexpr std::byte maximum_format_id = std::get<2>(values);
    static constexpr std::int8_t max_fixed_measure = std::get<int8_t>(values);  // maximum size/count for this format.

    static constexpr bool is_fixed =  size_len == 0;
    static constexpr bool variable_format = minimum_format_id != maximum_format_id;
    static constexpr format next_format = std::get<format>(values);
    static constexpr auto is_last = next_format == format::UNUSED;
    using next_traits = std::conditional_t<is_last, std::false_type, format_traits<next_format>>;

    static constexpr auto chain_size() {
        if constexpr (is_last) {
            return 1;
        } else {
            return next_traits::chain_size() + 1;
        }
    }

    static constexpr auto chain() {
        auto result = std::array<format, chain_size()>{};
        auto it = std::begin(result);
        *it = value;
        it++;
        if constexpr (!is_last) {
            std::ranges::copy(next_traits::chain(), it);
        }
        return result;
    }
};

template <is_packable VALUE_TYPE>
static constexpr std::uintmax_t measure(const VALUE_TYPE& value)
{
    if constexpr (std::ranges::sized_range<VALUE_TYPE>) {
        return value.size();
    } else if constexpr (std::integral<VALUE_TYPE>) {
        return sizeof(VALUE_TYPE);
    }
}

template <typename TYPE>
static constexpr auto format_type_of = []() {
    if constexpr (is_str_like<TYPE>) {
        return format_type::STR;
    } else if constexpr (is_array_like<TYPE> || is_decomposable<TYPE>) {
        return format_type::ARRAY;
    } else if constexpr (is_map_like<TYPE>) {
        return format_type::MAP;
    } else if constexpr (std::integral<TYPE>) {
        return format_type::INTEGER;
    } else if constexpr (std::same_as<TYPE, bool>) {
        return format_type::BOOL;
    } else if constexpr (std::floating_point<TYPE>) {
        return format_type::FLOAT;
    } else if constexpr (std::same_as<TYPE, std::nullptr_t> || std::same_as<TYPE, std::nullopt_t> || std::same_as<TYPE, std::monostate>) {
        return format_type::VOID;
    } else if constexpr (std::same_as<TYPE, std::chrono::system_clock::time_point>) {
        return format_type::EXT;
    } else {
        return format_type::UNKNOWN;
    }
}();

template <format_type TYPE>
struct format_type_traits {
    using seed_format_traits = format_traits<first_format<TYPE>>;
    static constexpr auto possible_formats = seed_format_traits::chain();
    static constexpr auto format_count = possible_formats.size();

    template <int n = 0, template<format> typename T>
    requires std::predicate<T<possible_formats[n]>> && std::default_initializable<T<possible_formats[n]>>
    static constexpr auto find_format() {
        using traits = format_traits<format_type_traits::possible_formats[n]>;
        static constexpr auto format = traits::value;
        if constexpr (T<format>{}()) {
            return traits::value;
        } else if constexpr (n >= format_count) {
            return format::UNUSED;
        } else {
            return format_traits_find<n+1>();
        }
    }
};

template <typename TYPE>
using format_type_traits_of = format_type_traits<format_type_of<TYPE>>;

template <is_packable VALUE_TYPE>
static constexpr std::byte format_id(const VALUE_TYPE& value)
{
    using traits = format_type_traits_of<VALUE_TYPE>;
    return traits::format_find([](format fmt)
}


static_assert(std::ranges::size(format_type_traits<format_type::ARRAY>::possible_formats) == 3);
static_assert(format_type_traits<format_type::VOID>::format_count == 1);

}
#endif // INCLUDED_FORMAT_HPP
