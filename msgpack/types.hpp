#ifndef INCLUDED_TYPES_HPP
#define INCLUDED_TYPES_HPP

#include <bit>
#include <concepts>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace vb::msgpack {

struct numeric
{
    using value_type = std::variant<int8_t,
                                    uint8_t,
                                    int16_t,
                                    uint16_t,
                                    int32_t,
                                    uint32_t,
                                    int64_t,
                                    uint64_t,
                                    float,
                                    double>;
    value_type value;

    template<typename T>
    requires(std::constructible_from<value_type, T>)
    static constexpr auto index_of = value_type{T{}}.index();

    explicit numeric(std::constructible_from<value_type> auto val)
      : value{ val }
    {
    }

    auto size() const
    {
        return std::visit([]<typename T>(const T&) { return sizeof(T); },
                          value);
    }

    operator std::uintmax_t()
    {
        return std::visit(
          []<typename T>(const T& value) {
              if constexpr (std::integral<T>) {
                  auto val = std::bit_cast<std::make_unsigned_t<T>>(value);
                  return static_cast<std::uintmax_t>(val);
              } else {
                  return static_cast<std::uintmax_t>(value);
              }
              return std::uintmax_t{ 0 };
          },
          value);
    }
};

template<unsigned LEN, bool IS_SIGNED = false>
using integer = std::variant_alternative_t<std::bit_width(LEN/16)*2+(IS_SIGNED?0:1), numeric::value_type>;
static_assert(std::same_as<integer<64>, std::uint64_t>);
static_assert(std::same_as<integer<32, true>, std::int32_t>);

template<unsigned LEN>
using floating = std::variant_alternative_t<std::bit_width(LEN/64)+numeric::index_of<float>, numeric::value_type>;

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
    std::same_as<TYPE, numeric> ||
    std::constructible_from<numeric, TYPE>;

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
