#ifndef INCLUDED_MSGPACK_HPP
#define INCLUDED_MSGPACK_HPP

#include <concepts>
#include <cstddef>

#include <iterator>
#include <ranges>

#include "./msgpack/format.hpp"
#include "./msgpack/types.hpp"

namespace vb::msgpack {

template <typename TARGET>
concept is_packing_target = std::output_iterator<TARGET, std::byte>;

template <typename SOURCE>
concept is_packing_source = std::ranges::range<SOURCE> && std::same_as<std::ranges::range_value_t<SOURCE>, std::byte>;

template <typename TYPE>
concept is_packable = is_decomposable<TYPE> || is_basic_type<TYPE>;

//auto as_bytes_view(std::

template<is_packable TYPE_T, is_packing_target TARGET_T>
void pack(const TYPE_T& obj, TARGET_T& out)
{
    using pack_type_traits = format_type_traits<format_type_for(obj)>;

    auto size = pack_type_traits::measure(obj);

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
