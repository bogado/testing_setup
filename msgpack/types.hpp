#ifndef INCLUDED_TYPES_HPP
#define INCLUDED_TYPES_HPP

#include "./byte_view.hpp"

#include <sys/types.h>
#include <any>
#include <bit>
#include <array>
#include <compare>
#include <concepts>
#include <cstdint>
#include <map>
#include <ranges>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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

template<type_t TYPE>
using standard_type = std::conditional_t<
  TYPE == type_t::INTEGER,
  std::intmax_t,
  std::conditional_t<
    TYPE == type_t::FLOAT,
    double,
    std::conditional_t<
      TYPE == type_t::BOOL,
      bool,
      std::conditional_t<
        TYPE == type_t::STR,
        std::string,
        std::conditional_t<
          TYPE == type_t::ARRAY,
          std::vector<std::any>,
          std::conditional_t<
            TYPE == type_t::MAP,
            std::map<std::any, std::any>,
            std::conditional_t<
              TYPE == type_t::EXT,
              std::any,
              std::conditional_t<TYPE == type_t::BIN,
                                 std::vector<std::byte>,
                                 std::conditional<TYPE == type_t::VOID,
                                                  std::nullptr_t,
                                                  std::false_type>>>>>>>>>;


template <typename... TYPEs>
constexpr auto variant_sizeof(std::variant<TYPEs...> var) 
{
    return std::visit([]<typename T>(T) { return sizeof(T); }, var);
}

template <typename T1, typename ... Ts>
constexpr auto not_anyof = ((!std::same_as<T1, Ts>) && ... && true);

template <typename T, typename... Ts>
constexpr auto all_different = []() {
    if constexpr(sizeof...(Ts) > 0) {
        return not_anyof<T, Ts...> && all_different<Ts...>;
    } else {
        return true;
    }
}();

template <typename... Ts>
requires (all_different<Ts...>)
using type_set = std::variant<Ts...>;

using int_value =
  type_set<std::int8_t, std::int16_t, std::int32_t, std::int64_t>;

using unsigned_value =
  type_set<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>;

using float_value = type_set<float, double>;

template <int LEN>
constexpr unsigned bit_size = (LEN == 8 || LEN == 16 || LEN == 32 || LEN == 64)?LEN:8;

template <int LEN>
constexpr unsigned var_index = std::bit_width(bit_size<LEN>/8)-1;

template<int LEN, bool IS_SIGNED = false>
using integer = std::conditional_t<
  LEN == bit_size<LEN>,
  std::variant_alternative_t<
    var_index<LEN>,
    std::conditional_t<IS_SIGNED, int_value, unsigned_value>>,
  std::conditional_t<IS_SIGNED, int, unsigned>>;

static_assert(std::same_as<integer<64>, std::uint64_t>);
static_assert(std::same_as<integer<32, true>, std::int32_t>);

template<unsigned LEN>
using floating = std::variant_alternative_t<std::bit_width(LEN/64), float_value>;

static_assert(std::same_as<floating<32>, float>);

template<std::uint8_t SIZE>
struct ext {
    std::byte type_spec;
    std::array<std::byte, SIZE> data;
};

template<typename TYPE>
concept is_str_like =
  std::same_as<TYPE, std::string> || std::same_as<TYPE, std::string_view> ||
  std::same_as<TYPE, const char *>;

template<typename TYPE>
concept is_numeric_like =
    std::same_as<TYPE, int_value> ||
    std::constructible_from<int_value, TYPE> ||
    std::same_as<TYPE, float_value> ||
    std::constructible_from<float_value, TYPE>; 

template<typename CLASS_T>
concept is_decomposable = requires(const CLASS_T val) {
    { std::tuple_size_v<CLASS_T> } -> std::unsigned_integral;
    { std::get<0>(val) };
};

template<typename MAP>
concept is_map_like = std::ranges::range<MAP> && requires(const MAP& map) {
            { map.begin()->first };
            { map.begin()->second };
};

static_assert(is_map_like<std::map<std::string, int>>);

template<typename ARRAY>
concept is_array_like =
  is_decomposable<ARRAY> ||
  (std::ranges::range<ARRAY> && !is_map_like<ARRAY> && !is_str_like<ARRAY>);

template<typename T>
concept is_ext_like = false;

template<typename BUFFER_LIKE>
concept is_buffer_like =
  is_array_like<BUFFER_LIKE> &&
  std::same_as<std::make_unsigned_t<std::ranges::range_value_t<BUFFER_LIKE>>,
               unsigned char>;

template<typename PACKABLE>
concept is_packable =
  is_array_like<PACKABLE> || is_map_like<PACKABLE> || is_str_like<PACKABLE> ||
  std::is_arithmetic_v<PACKABLE> || is_decomposable<PACKABLE>;
}

template <typename TARGET>
concept is_packing_target = std::output_iterator<TARGET, std::byte>;

template <typename SOURCE>
concept is_packing_source = std::ranges::range<SOURCE> && std::same_as<std::ranges::range_value_t<SOURCE>, std::byte>;

#endif // INCLUDED_TYPES_HPP
