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

template <typename T>
constexpr format_type as_format_type = format_type::UNKNOWN;

template <std::integral INTEGER_T>
constexpr format_type as_format_type<INTEGER_T> = format_type::INTEGER;

template <std::floating_point FLOATING_T>
constexpr format_type as_format_type<FLOATING_T> = format_type::FLOAT;

template <is_str_like STRING_T>
constexpr format_type as_format_type<STRING_T> = format_type::STR;

template <is_array_like ARRAY_T>
constexpr format_type as_format_type<ARRAY_T> = format_type::ARRAY;

template <is_map_like MAP_T>
constexpr format_type as_format_type<MAP_T> = format_type::MAP;

constexpr auto format_type_for(const auto& value) noexcept {
    return as_format_type<decltype(value)>;
}

template <typename TYPE>
std::intmax_t count_for(const TYPE& value) {
    if constexpr (is_decomposable<TYPE>) {
        return std::tuple_size_v<TYPE>;
    } else {
        switch(format_type_for(value)) {
        case format_type::INTEGER:
        case format_type::FLOAT:
            return static_cast<std::intmax_t>(size_of(value));
        case format_type::STR:
        case format_type::MAP:
        case format_type::ARRAY:
            return std::size(value);
       }
    }
}
