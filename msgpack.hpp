#ifndef INCLUDED_MSGPACK_HPP
#define INCLUDED_MSGPACK_HPP

#include "./msgpack/format.hpp"

#include <algorithm>
#include <cstddef>

namespace vb::msgpack {

template <is_packable TYPE>
constexpr auto unpack(is_packing_source auto source, TYPE& result)
{
    auto traits = format::classification{source.first};
    if (!traits.accepts<TYPE>()) {
        return std::ranges::all_of(source);
    } else if (traits.is_value()) {
        result = traits.value();
        return std::ranges::subrange(source, 1);
    } else if (traits.count_length() > 0) {
        std::int64_t count{0};
        auto rest = traits.read_count(source, count);
        return traits.read_data(rest, count, result);
    } 
    return traits.read_data(source, result);
}

}
#endif // INCLUDED_MSGPACK_HPP
