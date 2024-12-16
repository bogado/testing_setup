#include <string>
#include <string_view>

#include "./format.hpp"

template <typename TYPE>
concept is_basic_type = std::is_arithmetic_v<TYPE>;

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


