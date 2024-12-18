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

namespace vb::msgpack {

enum class format : std::uint8_t
{
    POSITIVE_FIX_INT,
    FIX_MAP,
    FIX_ARRAY,
    FIX_STR,
    NIL,
    UNUSED,
    BOOLEAN,
    BIN_8,
    BIN_16,
    BIN_32,
    EXT_8,
    EXT_16,
    EXT_32,
    FLOAT_32,
    FLOAT_64,
    UINT_8,
    UINT_16,
    UINT_32,
    UINT_64,
    INT_8,
    INT_16,
    INT_32,
    INT_64,
    FIXEXT_1,
    FIXEXT_2,
    FIXEXT_4,
    FIXEXT_8,
    FIXEXT_16,
    STR_8,
    STR_16,
    STR_32,
    ARRAY_16,
    ARRAY_32,
    MAP_16,
    MAP_32,
    NEGATIVE_FIX_INT
};

enum class format_type : std::uint8_t
{
    INTEGER = std::to_underlying(format::NEGATIVE_FIX_INT),
    BOOL = std::to_underlying(format::BOOLEAN),
    FLOAT = std::to_underlying(format::FLOAT_32),
    STR = std::to_underlying(format::FIX_STR),
    ARRAY = std::to_underlying(format::FIX_ARRAY),
    MAP = std::to_underlying(format::FIX_MAP),
    EXT = std::to_underlying(format::FIXEXT_1),
    VOID = std::to_underlying(format::NIL),
    BIN = std::to_underlying(format::BIN_8),
    UNKNOWN = std::to_underlying(format::UNUSED)
};

template <format_type FORMAT_TYPE>
constexpr auto first_format = format{std::to_underlying(FORMAT_TYPE)};

constexpr auto format_type_for(format fmt) 
{
    using enum format;
    using enum format_type;

    if (fmt == POSITIVE_FIX_INT || fmt == NEGATIVE_FIX_INT || (fmt >= UINT_8 && fmt <= INT_64)) {
        return INTEGER;
    } else if (fmt == BOOLEAN) {
        return BOOL;
    } else if (fmt == FLOAT_32 || fmt == FLOAT_64) {
        return FLOAT;
    } else if (fmt == FIX_STR || (fmt >= STR_8 && fmt <= STR_16)) {
        return STR;
    } else if (fmt == FIX_ARRAY || fmt == ARRAY_16 || fmt == ARRAY_32) {
        return ARRAY;
    } else if (fmt == FIX_MAP || fmt == MAP_16 || fmt == MAP_32) {
        return MAP;
    } else if (
     (fmt >= FIXEXT_1 && fmt <= FIXEXT_16) || (fmt >= EXT_8 && fmt <= EXT_32)) {
        return EXT;
    } else if (fmt == NIL) {
        return VOID;
    } else if (fmt >= BIN_8 && fmt <= BIN_32) {
        return BIN;
    } 
    return UNKNOWN;
}

template <format FORMAT>
struct format_traits
{
    static constexpr format value = FORMAT;
    static constexpr format_type type = format_type_for(FORMAT);
private:
    using values_t = std::tuple<std::uint8_t, std::byte, std::byte, std::int8_t, format>;
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
