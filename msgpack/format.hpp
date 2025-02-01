#ifndef INCLUDED_FORMAT_HPP
#define INCLUDED_FORMAT_HPP

#include "./format_type.hpp"

#include <cstddef>
#include <ranges>
#include <source_location>
#include <stdexcept>
#include <utility>
#include <variant>

namespace vb::msgpack::format {

struct classification
{
    using enum type_t;
    using enum category_t;

    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    using POSITIVE_FIX_INT = format::traits<INTEGER, VALUE | NUMERIC, 0, 0x7f>;
    using FIX_MAP = format::traits<MAP, CONTAINER | FIXED, 0x80, 0xf>;
    using FIX_ARRAY = format::traits<ARRAY, CONTAINER | FIXED, 0x90, 0xf>;
    using FIX_STR = format::traits<STR, CONTAINER | FIXED, 0xa0, 0x1f>;
    using NIL = format::traits<VOID, VALUE, 0xc0, 0>;
    using UNUSED = format::traits<VOID, UNKNOWN, 0xc1, 1>;
    using FALSE = format::traits<BOOL, VALUE | CONSTANT, 0xc2, 2>;
    using TRUE = format::traits<BOOL, VALUE | CONSTANT, 0xc3, 1>;
    using BIN_8 = format::traits<BIN, BITS, 0xc4, 8>;
    using BIN_16 = format::traits<BIN, BITS, 0xc5, 16>;
    using BIN_32 = format::traits<BIN, BITS, 0xc6, 32>;
    using EXT_8 = format::traits<EXT, BITS, 0xc7, 8>;
    using EXT_16 = format::traits<EXT, BITS, 0xc8, 16>;
    using EXT_32 = format::traits<EXT, BITS, 0xc9, 32>;
    using FLOAT_32 = format::traits<FLOAT, NUMERIC | BITS | SIGNED, 0xca, 32>;
    using FLOAT_64 = format::traits<FLOAT, NUMERIC | BITS | SIGNED, 0xcb, 64>;
    using UINT_8 = format::traits<INTEGER, NUMERIC | BITS | FIXED, 0xcc, 8>;
    using UINT_16 = format::traits<INTEGER, NUMERIC | BITS | FIXED, 0xcd, 16>;
    using UINT_32 = format::traits<INTEGER, NUMERIC | BITS | FIXED, 0xce, 32>;
    using UINT_64 = format::traits<INTEGER, NUMERIC | BITS | FIXED, 0xcf, 8>;
    using INT_8 =
      format::traits<INTEGER, NUMERIC | BITS | SIGNED | FIXED, 0xd0, 8>;
    using INT_16 =
      format::traits<INTEGER, NUMERIC | BITS | SIGNED | FIXED, 0xd1, 16>;
    using INT_32 =
      format::traits<INTEGER, NUMERIC | BITS | SIGNED | FIXED, 0xd2, 32>;
    using INT_64 =
      format::traits<INTEGER, NUMERIC | BITS | SIGNED | FIXED, 0xd3, 64>;
    using FIXEXT_1 = format::traits<EXT, FIXED | SIZED, 0xd4, 1>;
    using FIXEXT_2 = format::traits<EXT, FIXED | SIZED, 0xd5, 2>;
    using FIXEXT_4 = format::traits<EXT, FIXED | SIZED, 0xd6, 4>;
    using FIXEXT_8 = format::traits<EXT, FIXED | SIZED, 0xd7, 8>;
    using FIXEXT_16 = format::traits<EXT, FIXED | SIZED, 0xd8, 16>;
    using STR_8 = format::traits<STR, CONTAINER | BITS, 0xd9, 8>;
    using STR_16 = format::traits<STR, CONTAINER | BITS, 0xda, 16>;
    using STR_32 = format::traits<STR, CONTAINER | BITS, 0xdb, 32>;
    using ARRAY_16 = format::traits<ARRAY, CONTAINER | BITS, 0xdc, 16>;
    using ARRAY_32 = format::traits<ARRAY, CONTAINER | BITS, 0xdd, 32>;
    using MAP_16 = format::traits<MAP, CONTAINER | BITS, 0xde, 16>;
    using MAP_32 = format::traits<MAP, CONTAINER | BITS, 0xdf, 32>;
    using NEGATIVE_FIX_INT =
      format::traits<INTEGER, NUMERIC | SIGNED | VALUE, 0xe0, 0x1f>;

    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
    using traits_type = std::variant<
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
                                     NEGATIVE_FIX_INT,
                                     std::monostate>;

    static constexpr auto formats_count = std::variant_size_v<traits_type>;

    template<std::integral auto I>
    requires(I >= 0 && I < formats_count)
    using trait_num = std::variant_alternative_t<I, traits_type>;

    template <unsigned INDICE = 0>
    constexpr traits_type traits_for(std::byte index)
    {
        if constexpr (!std::same_as<trait_num<INDICE>, std::monostate>) {
            if (trait_num<INDICE>::accepts(format::id_type{index})) {
                return trait_num<INDICE>{id_type{index}};
            }
            return traits_for<INDICE+1>(index);
        } else {
            return std::monostate{};
        }
    }

    traits_type traits;

    constexpr classification(std::byte val)
      : traits{ traits_for(val) }
    {}

    template <typename INVOCABLE_T, typename RESULT_T>
    constexpr auto visitor(INVOCABLE_T invocable, RESULT_T default_value, std::source_location location [[maybe_unused]] = std::source_location::current()) const {
        return std::visit([invocable, default_value, &location]<typename ARGUMENT_T>(ARGUMENT_T value) -> RESULT_T {
            if constexpr (std::same_as<std::monostate, ARGUMENT_T>) {
                throw std::domain_error("Invalid traits setup.");
            } else if constexpr (!std::invocable<INVOCABLE_T, ARGUMENT_T> ) {
                throw std::logic_error(std::string("Invalid visitor ") + location.function_name() + " for id: " + std::to_string(ARGUMENT_T::format.value()));
            } else if constexpr (std::same_as<std::invoke_result_t<INVOCABLE_T, ARGUMENT_T>, std::false_type>) {
                return default_value;
            } else {
                return invocable(value);
            }
        }, traits);
    }

    constexpr id_type main_format_id() const {
        return visitor([]<typename TYPE>(const TYPE&) {
            return TYPE::format;
        }, id_type{});
    }

    constexpr id_type format_id() const {
        return visitor([](const auto& format) {
            return format.actual;
        }, id_type{});
    }

    constexpr bool is(is_category_or_type auto val) const
    {
        return visitor([val]<typename TRAITS_T>(TRAITS_T) {
            return TRAITS_T::is(val); 
        }, false);
    }

    constexpr auto content_size() const {
        return visitor([]<typename TRAITS_T>(const TRAITS_T& t) {
                return t.content_size();
        }, 0uz);
    }

    constexpr auto count_length() const {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> std::size_t {
                return TRAITS_T::spec_length;
        }, 0);
    }

    constexpr auto is_value() const {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> bool {
            return TRAITS_T::is(VALUE);
        }, false);
    }

    constexpr auto value() const {
        return visitor([]<typename TRAITS_T>(TRAITS_T traits) -> std::optional<std::int8_t> {
            if constexpr (TRAITS_T::is(VOID)) {
                return {};
            } else {
               return traits.value();
            }
        }, std::optional<int8_t>{});
    }

    constexpr auto read_count(is_packing_source auto source, std::integral auto& count) const
    {
        auto result = std::ranges::subrange(source);
        auto size = count_length();
        if (count_length() == 0) {
            return result;
        }

        count = 0;
        for (auto value: source | std::views::take(size) |
         std::views::transform([](std::byte value) { return
             std::to_underlying(value); })) {
            count += value;
            count <<= 1;
        }
        return result.advance(size);
    }

    template <is_packing_source SOURCE_T, is_packable OUT_TYPE>
    constexpr auto read_data(is_packing_source auto source, OUT_TYPE& out_data) const
    {
        out_data = from_bytes<OUT_TYPE>(source);
        return std::ranges::subrange(std::begin(source) + sizeof(OUT_TYPE), std::end(source));
    }

    template <is_packable TYPE>
    constexpr bool accepts() const {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> bool {
            return TRAITS_T::template accepts_type<TYPE>;
        }, false);
    }
};

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
static_assert(classification::POSITIVE_FIX_INT::is(classification::VALUE));
static_assert(!classification::POSITIVE_FIX_INT::is(classification::CONSTANT));
static_assert(!classification::POSITIVE_FIX_INT::is(classification::SIGNED));
static_assert(classification::POSITIVE_FIX_INT::id_range.first.value() == 0);
static_assert(classification::POSITIVE_FIX_INT::id_range.second.value() == 0x7f);
static_assert(classification::POSITIVE_FIX_INT::accepts(3));
static_assert(!classification::POSITIVE_FIX_INT::accepts(130));
static_assert(classification::NEGATIVE_FIX_INT{0xff}.value() == -1);
static_assert(classification::NEGATIVE_FIX_INT::id_range.first.value() == 0xe0);
static_assert(classification::NEGATIVE_FIX_INT::id_range.second.value() == 0xff);
static_assert(classification::NEGATIVE_FIX_INT::accepts(0xff));
static_assert(classification::FALSE::accepts(0xc2));
static_assert(classification::FIX_STR::accepts(0xa2));
static_assert(classification{std::byte{0xa2}}.is(classification::STR));
static_assert(classification{std::byte{0xa2}}.content_size() == 2);
static_assert(classification::FIX_ARRAY::is(classification::CONTAINER));
static_assert(classification::FIX_ARRAY::is(classification::FIXED));
static_assert(classification{std::byte{0x93}}.content_size() == 3);
static_assert(classification{std::byte{0x93}}.accepts<std::array<int, 3>>());
static_assert(classification{std::byte{0x93}}.accepts<std::array<unsigned, 3>>());
static_assert(classification{std::byte{0xd9}}.main_format_id() == 0xd9);
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

}
#endif // INCLUDED_FORMAT_HPP
