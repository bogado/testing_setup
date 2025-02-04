#ifndef INCLUDED_FORMAT_HPP
#define INCLUDED_FORMAT_HPP

#include "./format_type.hpp"
#include <algorithm>

#include <cstddef>
#include <ranges>
#include <source_location>
#include <utility>
#include <variant>

namespace vb::msgpack::format {

struct classification
{
    using enum type_t;
    using enum category_t;

    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    using POSITIVE_FIX_INT = format::traits<INTEGER, range_spec<0x7f, VALUE | NUMERIC | UNSIGNED>, id_type{0}>;
    using FIX_MAP = format::traits<MAP, range_spec<0xf, CONTAINER | FIXED>, id_type{0x80}>;
    using FIX_ARRAY = format::traits<ARRAY, range_spec<0xf, CONTAINER | FIXED>, id_type{0x90}>;
    using FIX_STR = format::traits<STR, range_spec<0x1f, CONTAINER | FIXED>, id_type{0xa0}>;
    using NIL = format::traits<VOID, spec, id_type{0xc0}>;
    using UNUSED = format::traits<VOID, spec, id_type{0xc1}>;
    using FALSE = format::traits<BOOL, value_spec<false>, id_type{0xc2}>;
    using TRUE = format::traits<BOOL, value_spec<true>, id_type{0xc3}>;
    using BIN_8 = format::traits<BIN, bit_spec<8, BINARY>, id_type{0xc4}>;
    using BIN_16 = format::traits<BIN, bit_spec<16, BINARY >, id_type{0xc5}>;
    using BIN_32 = format::traits<BIN, bit_spec<32, BINARY>, id_type{0xc6}>;
    using EXT_8 = format::traits<EXT, bit_spec<8, BINARY>, id_type{0xc7}>;
    using EXT_16 = format::traits<EXT, bit_spec<16, BINARY>, id_type{0xc8}>;
    using EXT_32 = format::traits<EXT, bit_spec<32, BINARY>, id_type{0xc9}>;
    using FLOAT_32 = format::traits<FLOAT, bit_spec<32, NUMERIC>, id_type{0xca}>;
    using FLOAT_64 = format::traits<FLOAT, bit_spec<64, NUMERIC>, id_type{0xcb}>;
    using UINT_8 = format::traits<INTEGER, bit_spec<8,  NUMERIC | UNSIGNED>, id_type{0xcc}>;
    using UINT_16 = format::traits<INTEGER, bit_spec<16, NUMERIC | UNSIGNED>, id_type{0xcd}>;
    using UINT_32 = format::traits<INTEGER, bit_spec<32, NUMERIC | UNSIGNED>, id_type{0xce}>;
    using UINT_64 = format::traits<INTEGER, bit_spec<8, NUMERIC | UNSIGNED>, id_type{0xcf}>;
    using INT_8 =
      format::traits<INTEGER, bit_spec<8, NUMERIC>, id_type{0xd0}>;
    using INT_16 =
      format::traits<INTEGER, bit_spec<16, NUMERIC>, id_type{0xd1}>;
    using INT_32 =
      format::traits<INTEGER, bit_spec<32, NUMERIC>, id_type{0xd2}>;
    using INT_64 =
      format::traits<INTEGER, bit_spec<64, NUMERIC>, id_type{0xd3}>;
    using FIXEXT_1 = format::traits<EXT, byte_spec<1, STATIC>, id_type{0xd4}>;
    using FIXEXT_2 = format::traits<EXT, byte_spec<2, STATIC>, id_type{0xd5}>;
    using FIXEXT_4 = format::traits<EXT, byte_spec<4, STATIC>, id_type{0xd6}>;
    using FIXEXT_8 = format::traits<EXT, byte_spec<8, STATIC>, id_type{0xd7}>;
    using FIXEXT_16 = format::traits<EXT, byte_spec<16, STATIC>, id_type{0xd8}>;
    using STR_8 = format::traits<STR, bit_spec<8, CONTAINER>, id_type{0xd9}>;
    using STR_16 = format::traits<STR, bit_spec<16, CONTAINER>, id_type{0xda}>;
    using STR_32 = format::traits<STR, bit_spec<32, CONTAINER>, id_type{0xdb}>;
    using ARRAY_16 = format::traits<ARRAY, bit_spec<16, CONTAINER>, id_type{0xdc}>;
    using ARRAY_32 = format::traits<ARRAY, bit_spec<32, CONTAINER>, id_type{0xdd}>;
    using MAP_16 = format::traits<MAP, bit_spec<16, CONTAINER>, id_type{0xde}>;
    using MAP_32 = format::traits<MAP, bit_spec<32, CONTAINER>, id_type{0xdf}>;
    using NEGATIVE_FIX_INT =
      format::traits<INTEGER, range_spec<0x1f, NUMERIC | VALUE>, id_type{0xe0}>;

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

    template<typename INVOCABLE_T, typename RESULT_T>
    constexpr auto visitor(INVOCABLE_T invocable,
                           RESULT_T default_value,
                           std::source_location location [[maybe_unused]] =
                             std::source_location::current()) const
    {
        return std::visit(
          [invocable, default_value, &location]<typename ARGUMENT_T>(
            ARGUMENT_T value) -> RESULT_T {
              if constexpr (std::same_as<std::monostate, ARGUMENT_T>) {
                  throw std::domain_error("Invalid traits setup.");
              } else if constexpr (!std::invocable<INVOCABLE_T, ARGUMENT_T>) {
                  throw std::logic_error(
                    std::string("Invalid visitor ") + location.function_name() +
                    " for id: " + std::to_string(ARGUMENT_T::format.value()));
              } else if constexpr (std::same_as<
                                     std::invoke_result_t<INVOCABLE_T,
                                                          ARGUMENT_T>,
                                     std::false_type>) {
                  return default_value;
              } else {
                  return invocable(value);
              }
          },
          traits);
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

    constexpr auto length_size() const {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> std::size_t {
                return TRAITS_T::length_size;
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
        auto size = length_size();
        if (size == 0) {
            return result;
        }

        auto source_view = source | std::views::take(size) |
                                    std::views::transform([](std::byte value) {
                                        return std::to_underlying(value);
                                    });
        count =
          std::ranges::fold_left(source_view, 0,
                                  [](std::size_t count, auto value) {
                                      count <<= 8;
                                      count += value;
                                      return count;
                                  });
        return result.advance(size);
    }

    template <is_packing_source SOURCE_T, is_packable OUT_TYPE>
    constexpr auto read_data(is_packing_source auto source, OUT_TYPE& out_data) const
    {
        out_data = container_from_bytes<OUT_TYPE>(source);
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
static_assert(!classification::POSITIVE_FIX_INT::is(classification::STATIC));
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
static_assert(classification{std::byte{0xd9}}.traits.index() == 23);
static_assert(classification{std::byte{0xcd}}.content_size() == 2);
static_assert(classification{std::byte{0xd9}}.length_size() == 1);
static_assert(classification{std::byte{0xd9}}.content_size() == 0);
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

}
#endif // INCLUDED_FORMAT_HPP
