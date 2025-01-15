#ifndef INCLUDED_FORMAT_HPP
#define INCLUDED_FORMAT_HPP

#include <cstddef>
#include <variant>

#include "./format_type.hpp"

namespace vb::msgpack::format {

struct classification
{
    using enum type_t;
    using enum category_t;

    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    using POSITIVE_FIX_INT = format::traits<INTEGER, VALUE, 0, 0x7f>;
    using FIX_MAP = format::traits<MAP, CONTAINER | FIXED, 0x80, 0xf>;
    using FIX_ARRAY = format::traits<ARRAY, CONTAINER | FIXED, 0x90, 0xf>;
    using FIX_STR = format::traits<STR, CONTAINER | FIXED, 0xa0, 0x1f>;
    using NIL = format::traits<VOID, VALUE, 0xc0, 0>;
    using UNUSED = format::traits<VOID, UNKNOWN, 0xc1, 1>;
    using TRUE = format::traits<BOOL, VALUE, 0xc2, 1>;
    using FALSE = format::traits<BOOL, VALUE, 0xc3, 0>;
    using BIN_8 = format::traits<BIN, SIZED, 0xc4, 1>;
    using BIN_16 = format::traits<BIN, SIZED, 0xc5, 2>;
    using BIN_32 = format::traits<BIN, SIZED, 0xc6, 4>;
    using EXT_8 = format::traits<EXT, SIZED, 0xc7, 1>;
    using EXT_16 = format::traits<EXT, SIZED, 0xc8, 2>;
    using EXT_32 = format::traits<EXT, SIZED, 0xc9, 4>;
    using FLOAT_32 = format::traits<EXT, NUMERIC | SIGNED, 0xca, 1>;
    using FLOAT_64 = format::traits<EXT, NUMERIC | SIGNED, 0xcb, 1>;
    using UINT_8 = format::traits<INTEGER, NUMERIC | FIXED, 0xcc, 1>;
    using UINT_16 = format::traits<INTEGER, NUMERIC | FIXED, 0xcd, 2>;
    using UINT_32 = format::traits<INTEGER, NUMERIC | FIXED, 0xce, 4>;
    using UINT_64 = format::traits<INTEGER, NUMERIC | FIXED, 0xcf, 8>;
    using INT_8 = format::traits<INTEGER, NUMERIC | SIGNED | FIXED, 0xd0, 1>;
    using INT_16 = format::traits<INTEGER, NUMERIC | SIGNED | FIXED, 0xd1, 2>;
    using INT_32 = format::traits<INTEGER, NUMERIC | SIGNED | FIXED, 0xd2, 4>;
    using INT_64 = format::traits<INTEGER, NUMERIC | SIGNED | FIXED, 0xd3, 8>;
    using FIXEXT_1 = format::traits<EXT, FIXED, 0xd4, 1>;
    using FIXEXT_2 = format::traits<EXT, FIXED, 0xd5, 2>;
    using FIXEXT_4 = format::traits<EXT, FIXED, 0xd6, 4>;
    using FIXEXT_8 = format::traits<EXT, FIXED, 0xd7, 8>;
    using FIXEXT_16 = format::traits<EXT, FIXED, 0xd8, 16>;
    using STR_8 = format::traits<STR, CONTAINER, 0xd9, 1>;
    using STR_16 = format::traits<STR, CONTAINER, 0xda, 2>;
    using STR_32 = format::traits<STR, CONTAINER, 0xdb, 4>;
    using ARRAY_16 = format::traits<ARRAY, CONTAINER, 0xdc, 2>;
    using ARRAY_32 = format::traits<ARRAY, CONTAINER, 0xdd, 4>;
    using MAP_16 = format::traits<MAP, CONTAINER, 0xde, 2>;
    using MAP_32 = format::traits<MAP, CONTAINER, 0xdf, 4>;
    using NEGATIVE_FIX_INT =
      format::traits<INTEGER, SIGNED | VALUE, 0xe0, -0x1f>;

    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
    using traits_type = std::variant<std::monostate,
                                     POSITIVE_FIX_INT,
                                     FIX_MAP,
                                     FIX_ARRAY,
                                     FIX_STR,
                                     NIL,
                                     UNUSED,
                                     TRUE,
                                     FALSE,
                                     BIN_8,
                                     BIN_16,
                                     FLOAT_32,
                                     FLOAT_64,
                                     UINT_8,
                                     UINT_16,
                                     UINT_32,
                                     INT_8,
                                     INT_16,
                                     INT_32,
                                     INT_64,
                                     FIXEXT_1,
                                     FIXEXT_2,
                                     FIXEXT_4,
                                     FIXEXT_8,
                                     STR_8,
                                     STR_16,
                                     STR_32,
                                     ARRAY_16,
                                     ARRAY_32,
                                     MAP_16,
                                     MAP_32,
                                     NEGATIVE_FIX_INT>;

    static constexpr auto formats_count = std::variant_size_v<traits_type>;

    template<std::integral auto I>
    requires(I >= 0 && I < formats_count)
    using trait_num = std::variant_alternative_t<I, traits_type>;

    template <unsigned INDICE = formats_count-1>
    constexpr traits_type traits_for(std::byte index)
    {
        if constexpr (std::same_as<trait_num<INDICE>, std::monostate>) {
            return std::monostate{};
        } else {
            if (trait_num<INDICE>::accepts(format::id{index})) {
                return trait_num<INDICE>{};
            }
            return traits_for<INDICE-1>(index);
        }
    }

    traits_type traits;

    constexpr classification(std::byte val)
      : traits{ traits_for(val) }
    {}

    template <typename INVOCABLE_T, typename RESULT_T>
    constexpr auto visitor(INVOCABLE_T invocable, RESULT_T default_value) {
        return std::visit([invocable, default_value]<typename ARGUMENT_T>(ARGUMENT_T value) -> RESULT_T {
            if constexpr (std::same_as<std::monostate, ARGUMENT_T>) {
                throw std::logic_error("Invalid access");
            } else if constexpr (std::same_as<std::invoke_result_t<INVOCABLE_T, ARGUMENT_T>, std::false_type>) {
                return default_value;
            } else {
                return invocable(value);
            }
        }, traits);
    }

    constexpr auto content_size() {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> int {
                return TRAITS_T::content_size;
        }, 0);
    }

    constexpr auto count_length() {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> std::size_t {
                return TRAITS_T::count_bytes;
        }, 0);
    }

    constexpr auto is_value() {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> bool {
            return TRAITS_T::is(VALUE);
        }, false);
    }

    constexpr auto value() {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> std::optional<std::int8_t> {
            if constexpr (TRAITS_T::is(VOID)) {
                return {};
            } else {
               return TRAITS_T::value;
            }
        }, std::optional<int8_t>{});
    }

    template <is_packable TYPE>
    constexpr bool accepts() {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> bool {
            return TRAITS_T::template accepts_type<TYPE>;
        }, false);
    }
};

}
#endif // INCLUDED_FORMAT_HPP
