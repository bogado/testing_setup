#ifndef INCLUDED_TYPES_HPP
#define INCLUDED_TYPES_HPP

#include <bit>
#include <compare>
#include <concepts>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

namespace vb::msgpack {

template <typename NUMERICAL>
concept is_numeric = requires(const NUMERICAL val) {
    { val + val } -> std::convertible_to<NUMERICAL>;
    { val * val } -> std::convertible_to<NUMERICAL>;
    { 1   * val } -> std::convertible_to<NUMERICAL>;
    { 1.0 * val } -> std::convertible_to<NUMERICAL>;
    std::three_way_comparable_with<NUMERICAL, NUMERICAL>;
    std::convertible_to<NUMERICAL, std::int64_t> || std::convertible_to<NUMERICAL, std::uint64_t>;
    std::convertible_to<NUMERICAL, double>;
};

template<is_numeric... NUMERICs>
struct numeric_union
{
    using value_type = std::variant<NUMERICs...>;

    static constexpr auto type_count =  std::variant_size_v<value_type>;

    template <std::integral auto I>
    requires(I >= 0 && I < type_count)
    using int_type = std::variant_alternative_t<I, value_type>;

    auto as_int() const
    {
        return std::optional{std::visit(
          []<std::integral INT_T>(const INT_T& val) {
              return static_cast<std::int64_t>(val);
          }, value)};
    }

    auto as_unsigned() const
    {
        return std::optional{std::visit(
          []<std::integral INT_T>(const INT_T& val) {
              return static_cast<std::uint64_t>(val);
          }, value)};
    }

    auto as_double() const
    {
        return std::visit(
          []<std::integral INT_T>(const INT_T& val) {
              return static_cast<double>(val);
          }, value);
    }

    template <std::convertible_to<double> DOUBLE>
    requires(!std::is_integral_v<DOUBLE>)
    numeric_union operator* (DOUBLE& other) const {
        return std::visit(
         [&]<typename T>(const T& val) {
             return numeric_union{val * static_cast<double>(other)};
         }, value);
    }

    numeric_union operator* (const std::convertible_to<std::int64_t> auto& other) const {
        return std::visit(
         [&]<typename T>(const T& val) {
             return numeric_union{val * static_cast<std::int64_t>(other)};
         }, value);
    }

    friend numeric_union operator* (const std::convertible_to<int> auto& other, const numeric_union& self) {
        return self * other;
    }

    numeric_union operator* (const numeric_union& other) const {
        return std::visit(
         [&]<typename T>(const T& val) {
             if constexpr (std::is_floating_point_v<T>) {
                 return numeric_union{val * other.as_double().value()};
             } else if constexpr (std::is_integral_v<T>) {
                 return numeric_union{val * other.as_unsigned()};
             } else {
                 return numeric_union{val * other};
             }
         }, value);
    }

    numeric_union operator/ (const numeric_union& other) const {
        return std::visit(
         [&]<typename T>(const T& val) {
             if constexpr (std::is_floating_point_v<T>) {
                 return numeric_union{val / other.as_double()};
             } else if constexpr (std::is_integral_v<T>) {
                 return numeric_union{val / other.as_int()};
             } else {
                 return numeric_union{val / other};
             }
         }, value);
    }

    numeric_union operator+ (const numeric_union& other) const {
        return std::visit(
         [&]<typename T>(const T& val) {
             if constexpr (std::is_floating_point_v<T>) {
                 return numeric_union{static_cast<T>(val + other.as_double())};
             } else if constexpr (std::is_integral_v<T>) {
                 return numeric_union{static_cast<T>(val + other.as_int())};
             } else {
                 return numeric_union{val + other};
             }
         }, value);
    }

    numeric_union operator- (const numeric_union& other) const {
        return std::visit(
         [&]<typename T>(const T& val) {
             if constexpr (std::is_floating_point_v<T>) {
                 return numeric_union{static_cast<T>(val - other.as_double())};
             } else if constexpr (std::is_signed_v<T>) {
                 return numeric_union{static_cast<T>(val - other.as_int())};
             } else {
                 return numeric_union{val - other};
            }
         }, value);
    }

    numeric_union operator- () const {
        return std::visit(
         [&]<typename T>(const T& val) {
             if constexpr (std::is_floating_point_v<T>) {
                 return numeric_union{static_cast<T>(-val)};
             } else if constexpr (std::is_signed_v<T>) {
                 return numeric_union{static_cast<T>(-val)};
             } else {
                 return numeric_union{-val};
            }
         }, value);
    }

    value_type value;

    template<typename T>
    requires(std::constructible_from<value_type, T>)
    static constexpr auto index_of = value_type{T{}}.index();

    explicit numeric_union(std::constructible_from<value_type> auto val)
      : value{ val }
    {
    }

    auto size() const
    {
        return std::visit([]<typename T>(const T&) { return sizeof(T); },
                          value);
    }

    explicit operator std::intmax_t()
    {
        return as_int();
    }
};

using int_value = numeric_union<std::int8_t,
                                    std::uint8_t,
                                    std::int16_t,
                                    std::uint16_t,
                                    std::int32_t,
                                    std::uint32_t,
                                    std::int64_t,
                                    std::uint64_t>;

static_assert(is_numeric<int_value>);

using float_value = numeric_union<float, double>;

static_assert(is_numeric<float_value>);

template <int LEN>
constexpr unsigned bit_size = (LEN == 8 || LEN == 16 || LEN == 32 || LEN == 64)?LEN:8;

template<int LEN, bool IS_SIGNED = false>
using integer = std::conditional_t<
  LEN == bit_size<LEN>,
  std::variant_alternative_t<
    std::bit_width(bit_size<LEN> / 16) * 2 + (IS_SIGNED ? 0 : 1),
    int_value::value_type>,
  std::conditional_t<IS_SIGNED, int, unsigned>>;

static_assert(std::same_as<integer<64>, std::uint64_t>);
static_assert(std::same_as<integer<32, true>, std::int32_t>);

template<unsigned LEN>
using floating = std::variant_alternative_t<std::bit_width(LEN/64)+float_value::index_of<float>, float_value::value_type>;

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
            { *map.begin().first() };
            { *map.begin().second() };
};

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

#endif // INCLUDED_TYPES_HPP
