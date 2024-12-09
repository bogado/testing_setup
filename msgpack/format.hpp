#ifndef INCLUDED_FORMAT_HPP
#define INCLUDED_FORMAT_HPP

#include <cstddef>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <utility>

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
    INTEGER,
    BOOL,
    FLOAT,
    STR,
    ARRAY,
    MAP,
    EXT,
    VOID,
    BIN,
    UNKNOWN
};

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
    constexpr static auto values = []()
    {
        using values_t = std::tuple<std::uint8_t, std::byte, std::byte, std::int64_t>;

        static constexpr auto variable = [](std::uint8_t len, int start) -> values_t {
            return values_t{0, static_cast<std::byte>(start), static_cast<std::byte>(start+len), 0};
        };

        static constexpr auto byte_count = [](std::integral auto count) -> values_t {
            return values_t{ static_cast<uint8_t>(count), std::byte{0}, std::byte{0}, std::size_t{ 1 } << count };
        };

        static constexpr auto fixed = [](std::integral auto maximum) -> values_t {
            return { 0, std::byte{0}, std::byte{0}, static_cast<std::size_t>(maximum) };
        };

        static constexpr auto table =
            std::array{
                variable(0x7f, 0),
                variable(0x0f, 0x80 ),
                variable(0x0f, 0x90),
                variable(0x1f, 0xa0),
                values_t{0, std::byte{0}, std::byte{0xc0}, 0}, values_t{0, std::byte{0}, std::byte{0xc1}, 0},
                variable(2, 0xc2), 
                byte_count(1),  byte_count(2), byte_count(4), 
                byte_count(1), byte_count(2),  byte_count(4),
                fixed(4), fixed(8),
                fixed(1),  fixed(2), fixed(4), fixed(8),
                fixed(1),  fixed(2), fixed(4), fixed(8),
                fixed(1),  fixed(2), fixed(4), fixed(8), fixed(16),
                byte_count(1), byte_count(2), byte_count(4),
                byte_count(2),  byte_count(4),
                byte_count(2), byte_count(4),
                variable(0x1f, -1) };

        return table.at(std::to_underlying(value));
    }();
public:

    static constexpr std::uint8_t size_len = std::get<0>(values); // Number of bytes the size takes.
    static constexpr std::byte minimum_format_id = std::get<1>(values);
    static constexpr std::byte maximum_format_id = std::get<3>(values);
    static constexpr std::int64_t max_value = std::get<4>(values);  // maximum size/count for this format.

    static constexpr bool is_fixed =  size_len == 0;
    static constexpr bool variable_format = minimum_format_id != maximum_format_id;
};

}

#endif // INCLUDED_FORMAT_HPP
