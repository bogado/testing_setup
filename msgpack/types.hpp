#ifndef INCLUDED_TYPES_HPP
#define INCLUDED_TYPES_HPP

#include <string>
#include <string_view>
#include <type_traits>

namespace vb::msgpack {

template <typename TYPE>
concept is_str_like = std::same_as<TYPE, std::string> || std::same_as<TYPE, std::string_view> || std::same_as<TYPE, const char*>;

template <typename CLASS_T>
concept is_decomposable = requires(const CLASS_T val) {
    { std::tuple_size_v<CLASS_T> } -> std::unsigned_integral;
    { std::get<0>(val) };
};

template <typename MAP>
concept is_map_like = std::ranges::range<MAP> && requires(const MAP& map) {
    { *map.begin().first() };
    { *map.begin().second() };
};

template<typename ARRAY>
concept is_array_like =
  is_decomposable<ARRAY> || (std::ranges::range<ARRAY> && !is_map_like<ARRAY> && !is_str_like<ARRAY>);

template<typename PACKABLE>
concept is_packable =
  is_array_like<PACKABLE> || is_map_like<PACKABLE> || is_str_like<PACKABLE> ||
  std::is_arithmetic_v<PACKABLE> || is_decomposable<PACKABLE>;

}

#endif // INCLUDED_TYPES_HPP
