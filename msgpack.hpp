#ifndef INCLUDED_MSGPACK_HPP
#define INCLUDED_MSGPACK_HPP

#include "./msgpack/format.hpp"

#include <algorithm>
#include <cstddef>

namespace vb::msgpack {

template <typename TARGET>
concept is_packing_target = std::output_iterator<TARGET, std::byte>;

template <typename SOURCE>
concept is_packing_source = std::ranges::range<SOURCE> && std::same_as<std::ranges::range_value_t<SOURCE>, std::byte>;

template <is_packable TYPE>
constexpr auto unpack(is_packing_source auto source)
{
    auto traits = format::classification{source.first};
    if (!traits.accepts<TYPE>()) {
        return std::pair{std::ranges::all_of(source), std::optional<TYPE>{}};
    } else if (traits.is_value()) {
        return std::pair{std::ranges::subrange(source, 1), traits.value()};
    } else if (traits.count_length() > 0) {
        

    }
}

}
#endif // INCLUDED_MSGPACK_HPP
