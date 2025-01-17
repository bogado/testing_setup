#ifndef INCLUDED_TYPES_HPP
#define INCLUDED_TYPES_HPP

#include <sys/types.h>
#include <bit>
#include <array>
#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace vb::msgpack {

template<typename NUMERICAL>
concept is_numeric = requires(const NUMERICAL val) {
    { val + val } -> std::convertible_to<NUMERICAL>;
    { val *val } -> std::convertible_to<NUMERICAL>;
    { 1 * val } -> std::convertible_to<NUMERICAL>;
    { 1.0 * val } -> std::convertible_to<NUMERICAL>;
} && std::three_way_comparable_with<NUMERICAL, NUMERICAL>;

template <typename... TYPEs>
constexpr auto variant_sizeof(std::variant<TYPEs...> var) 
{
    return std::visit([]<typename T>(T) { return sizeof(T); }, var);
}

template<is_numeric... NUMERICs>
struct numeric_union
{
    using value_type = std::variant<NUMERICs...>;


    static constexpr auto type_count = sizeof...(NUMERICs);

    static constexpr auto prototypes = []<std::size_t ... I>(std::index_sequence<I...>) {
        return std::array{value_type{std::variant_alternative_t<I, value_type>{}}...};
    }(std::make_index_sequence<type_count>{});

    static constexpr auto sizes = []() {
        return prototypes | std::views::transform([](const auto& prototype) {
            return variant_sizeof(prototype);
        });
    }();

    template <std::integral auto I>
        requires(I >= 0 && I < type_count)
        using int_type = std::variant_alternative_t<I, value_type>;

    constexpr auto as_int() const
    {
        return std::visit(
         []<typename VALUE_T>(const VALUE_T& val) {
             if constexpr (std::is_convertible_v<VALUE_T, std::intmax_t>) {
                 return static_cast<std::intmax_t>(val);
             } else {
                 return val.as_int();
             }
         }, value);
    }

    constexpr auto as_unsigned() const
    {
        return std::visit(
         []<typename VALUE_T>(const VALUE_T& val) {
             if constexpr (std::is_convertible_v<VALUE_T, std::uintmax_t>) {
                 return static_cast<std::uintmax_t>(val);
             } else {
                 return val.as_unsigned();
             }
         }, value);
    }

    constexpr auto as_double() const
    {
        return std::visit(
          [&]<typename VALUE_T>(const VALUE_T& val) {
              if constexpr (std::same_as<VALUE_T, double>) {
                  return val;
              } else if constexpr (std::is_convertible_v<VALUE_T, double>) {
                  return static_cast<double>(val);
              } else {
                  return static_cast<double>(as_int());
              }
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
              if constexpr (std::is_floating_point_v<T> ||
                            std::is_signed_v<T>)
              {
                  return numeric_union{ static_cast<T>(-val) };
              } else {
                  return numeric_union{ -val };
              }
          },
          value);
    }

    friend constexpr std::strong_ordering operator<=>(const numeric_union& a, const numeric_union& b) {
        return std::visit([&]<typename A, typename B>(const A& av, const B& bv) -> std::strong_ordering {
            if constexpr (auto b_value = static_cast<A>(bv); sizeof(A) < sizeof(B)) { // NOLINT(bugprone-signed-char-misuse)
                return b <=> a;
            } else if constexpr (std::same_as<A,B>) {
                return av <=> bv;
            } else if constexpr (std::is_signed_v<A> == std::is_signed_v<B>) {
                return b_value <=> av;
            } else if constexpr (std::is_signed_v<A>) {
                return av < 0 ? std::strong_ordering::less : av <=> b_value;
            } else {
                return bv > 0 ? std::strong_ordering::greater : av <=> b_value;
            }
        }, a.value, b.value);
    }

    friend constexpr bool operator==(const numeric_union& a, const numeric_union& b) {
        return (a <=> b) == 0;
    }

    value_type value;

    template<typename T>
    requires(std::constructible_from<value_type, T>)
    static constexpr auto index_of = value_type{T{}}.index();

    explicit constexpr numeric_union(std::convertible_to<value_type> auto val)
      : value{ val }
    {
    }

    auto size() const
    {
        return std::visit([]<typename T>(const T&) { return sizeof(T); },
                          value);
    }

    explicit constexpr operator std::int64_t()
    {
        return as_int();
    }

    explicit constexpr operator std::uint64_t()
    {
        return as_unsigned();
    }

    explicit constexpr operator double()
    {
        return as_double();
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

static_assert(int_value{std::int8_t{1}} == int_value{std::int16_t{1}});
static_assert(int_value{2}.as_double() == 2.0);
static_assert(int_value{2}.as_int() == 2);
static_assert(int_value{-2}.as_unsigned() == std::numeric_limits<uintmax_t>::max() - 1);
static_assert(static_cast<std::int64_t>(int_value{3}) == 3);
static_assert(static_cast<std::uint64_t>(int_value{4}) == 4);
static_assert(static_cast<double>(int_value{1}) == 1.0);

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
