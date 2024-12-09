#ifndef INCLUDED_MSGPACK_HPP
#define INCLUDED_MSGPACK_HPP

#include <algorithm>
#include <concepts>
#include <cstddef>

#include <iterator>
#include <ranges>
#include <type_traits>

#include "./msgpack/format.hpp"

namespace vb::msgpack {

template <typename TARGET>
concept is_packing_target = std::output_iterator<TARGET, std::byte>;

template <typename SOURCE>
concept is_packing_source = std::ranges::range<SOURCE> && std::same_as<std::ranges::range_value_t<SOURCE>, std::byte>;

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
  is_decomposable<ARRAY> || (std::ranges::range<ARRAY> && !is_map_like<ARRAY> &&
                             requires(const ARRAY& array) {
                                 { *array.begin() };
                             });

template <typename TYPE>
concept is_basic_type = std::is_arithmetic_v<TYPE>;

template <typename TYPE>
concept is_packable = is_decomposable<TYPE> || is_basic_type<TYPE>;


//auto as_bytes_view(std::

/*
template<is_packable TYPE_T, is_packing_target TARGET_T>
void pack(const TYPE_T& obj, TARGET_T& out)
{
    const auto frmt = format_class{obj};
    *out = frmt.id();
    out++;

    std::ranges::copy(frmt.count_repr(), out);
    if constexpr (packs_as_array<TYPE_T>) {
        for (const auto& item : obj) {
            pack(item, out);
        }
    } else if constexpr (packs_as_map<TYPE_T>) {
        for (const auto& [key, value] : obj) {
            pack(key, out);
            pack(value, out);
        }
    } else if constexpr (std::is_integral_v<TYPE_T>) {
        if (frmt.is_fixed()) {
            return;
        }
    }
} */

}
#endif // INCLUDED_MSGPACK_HPP
