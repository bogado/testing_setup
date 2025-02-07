#ifndef INCLUDED_FORMAT_HPP
#define INCLUDED_FORMAT_HPP

#include "./format_type.hpp"
#include "format_id.hpp"
#include "types.hpp"

#include <algorithm>
#include <concepts>
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
    using POSITIVE_FIX_INT = format::traits<INTEGER, range_spec<id_type{0}, 0x7f, VALUE | NUMERIC | UNSIGNED>>;
    using FIX_MAP = format::traits<MAP, range_spec<id_type{0x80}, 0xf, CONTAINER | FIXED>>;
    using FIX_ARRAY = format::traits<ARRAY, range_spec<id_type{0x90}, 0xf, CONTAINER | FIXED>>;
    using FIX_STR = format::traits<STR, range_spec<id_type{0xa0}, 0x1f, CONTAINER | FIXED>>;
    using NIL = format::traits<VOID, value_spec<id_type{0xc0}, nullptr>>;
    using UNUSED = format::traits<VOID, nil_spec<id_type{0xc1}>>;
    using FALSE = format::traits<BOOL, value_spec<id_type{0xc2}, false>>;
    using TRUE = format::traits<BOOL, value_spec<id_type{0xc3}, true>>;
    using BIN_8 = format::traits<BIN, bit_spec<id_type{0xc4}, 8, BINARY>>;
    using BIN_16 = format::traits<BIN, bit_spec<id_type{0xc5}, 16, BINARY >>;
    using BIN_32 = format::traits<BIN, bit_spec<id_type{0xc6}, 32, BINARY>>;
    using EXT_8 = format::traits<EXT, bit_spec<id_type{0xc7}, 8, BINARY>>;
    using EXT_16 = format::traits<EXT, bit_spec<id_type{0xc8}, 16, BINARY>>;
    using EXT_32 = format::traits<EXT, bit_spec<id_type{0xc9}, 32, BINARY>>;
    using FLOAT_32 = format::traits<FLOAT, bit_spec<id_type{0xca}, 32, NUMERIC>>;
    using FLOAT_64 = format::traits<FLOAT, bit_spec<id_type{0xcb}, 64, NUMERIC>>;
    using UINT_8 = format::traits<INTEGER, bit_spec<id_type{0xcc}, 8,  NUMERIC | UNSIGNED>>;
    using UINT_16 = format::traits<INTEGER, bit_spec<id_type{0xcd}, 16, NUMERIC | UNSIGNED>>;
    using UINT_32 = format::traits<INTEGER, bit_spec<id_type{0xce}, 32, NUMERIC | UNSIGNED>>;
    using UINT_64 = format::traits<INTEGER, bit_spec<id_type{0xcf}, 8, NUMERIC | UNSIGNED>>;
    using INT_8 =
      format::traits<INTEGER, bit_spec<id_type{0xd0}, 8, NUMERIC>>;
    using INT_16 =
      format::traits<INTEGER, bit_spec<id_type{0xd1}, 16, NUMERIC>>;
    using INT_32 =
      format::traits<INTEGER, bit_spec<id_type{0xd2}, 32, NUMERIC>>;
    using INT_64 =
      format::traits<INTEGER, bit_spec<id_type{0xd3}, 64, NUMERIC>>;
    using FIXEXT_1 = format::traits<EXT, spec<id_type{0xd4}, 1, STATIC>>;
    using FIXEXT_2 = format::traits<EXT, spec<id_type{0xd5}, 2, STATIC>>;
    using FIXEXT_4 = format::traits<EXT, spec<id_type{0xd6}, 4, STATIC>>;
    using FIXEXT_8 = format::traits<EXT, spec<id_type{0xd7}, 8, STATIC>>;
    using FIXEXT_16 = format::traits<EXT, spec<id_type{0xd8}, 16, STATIC>>;
    using STR_8 = format::traits<STR, bit_spec<id_type{0xd9}, 8, CONTAINER>>;
    using STR_16 = format::traits<STR, bit_spec<id_type{0xda}, 16, CONTAINER>>;
    using STR_32 = format::traits<STR, bit_spec<id_type{0xdb}, 32, CONTAINER>>;
    using ARRAY_16 = format::traits<ARRAY, bit_spec<id_type{0xdc}, 16, CONTAINER>>;
    using ARRAY_32 = format::traits<ARRAY, bit_spec<id_type{0xdd}, 32, CONTAINER>>;
    using MAP_16 = format::traits<MAP, bit_spec<id_type{0xde}, 16, CONTAINER>>;
    using MAP_32 = format::traits<MAP, bit_spec<id_type{0xdf}, 32, CONTAINER>>;
    using NEGATIVE_FIX_INT =
      format::traits<INTEGER, range_spec<id_type{0xe0}, 0x1f, NUMERIC | VALUE>>;

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
    constexpr traits_type traits_for(id_type index)
    {
        if constexpr (!std::same_as<trait_num<INDICE>, std::monostate>) {
            if (trait_num<INDICE>::accepts(index)) {
                return trait_num<INDICE>{id_type{index}};
            }
            return traits_for<INDICE+1>(index);
        } else {
            return std::monostate{};
        }
    }

    traits_type traits;

    explicit constexpr classification(id_type val)
      : traits{traits_for(val) }
    {}

    template<typename T>
    requires(std::constructible_from<id_type, T>)
    explicit constexpr classification(T val)
        : classification{id_type{val}}
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
                  throw std::domain_error(std::string("Invalid traits setup : ") + location.function_name());
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

    template <is_packable TYPE>
    constexpr bool accepts() const {
        return visitor([]<typename TRAITS_T>(TRAITS_T) -> bool {
            return TRAITS_T::template accepts_type<TYPE>;
        }, false);
    }
};

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
namespace test {
using namespace format::literals;
static_assert(classification::POSITIVE_FIX_INT::is(category_t::VALUE));
static_assert(!classification::POSITIVE_FIX_INT::is(category_t::STATIC));
static_assert(classification::POSITIVE_FIX_INT::id_range.first.value() == 0);
static_assert(classification::POSITIVE_FIX_INT::id_range.second.value() == 0x7f);
static_assert(classification::POSITIVE_FIX_INT::accepts(3_id));
static_assert(!classification::POSITIVE_FIX_INT::accepts(130_id));
static_assert(classification::NEGATIVE_FIX_INT{0xff_id}.value() == -1);
static_assert(classification::NEGATIVE_FIX_INT::id_range.first.value() == 0xe0);
static_assert(classification::NEGATIVE_FIX_INT::id_range.second.value() == 0xff);
static_assert(classification::NEGATIVE_FIX_INT::accepts(id_type{0xff}));
static_assert(classification::FALSE::accepts(0xc2_id));
static_assert(classification::FIX_STR::accepts(0xa2_id));
static_assert(classification{id_type{0xa2}}.is(classification::STR));
static_assert(classification{id_type{0xa2}}.content_size() == 2);
static_assert(classification::FIX_ARRAY::is(classification::CONTAINER));
static_assert(classification::FIX_ARRAY::is(classification::FIXED));
static_assert(classification{id_type{0x93}}.content_size() == 3);
static_assert(classification{id_type{0x93}}.accepts<std::array<int, 3>>());
static_assert(classification{id_type{0x93}}.accepts<std::array<unsigned, 3>>());
static_assert(classification{id_type{0xd9}}.traits.index() == 23);
static_assert(classification{id_type{0xcd}}.content_size() == 2);
static_assert(classification{id_type{0xd9}}.length_size() == 1);
static_assert(classification{id_type{0xd9}}.content_size() == 0);
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}

}
#endif // INCLUDED_FORMAT_HPP
