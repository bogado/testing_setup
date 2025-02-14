#include "msgpack/reader.hpp"
    
#include <iostream>
#include <ranges>
#include <string>
#include <cctype>
#include <print>
#include <vector>

namespace message
{

enum class type : unsigned {
    REQUEST = 0,
    RESPONSE = 1,
    NOTIFICATION = 2
};
}

template <typename... ARGs>
struct payload {
    static inline auto last_id = std::uint32_t{0};

    constexpr payload(message::type mtype, const std::string& cmd, ARGs... args)
        : my_type{mtype}
        , id{last_id}
        , name{cmd}
        , arguments{std::move(args)...}
    {
        last_id++;
    }

    message::type my_type;
    std::uint32_t id;
    std::string name;
    std::tuple<ARGs...> arguments;

    constexpr void pack(this auto&& self, auto &pack) {
        pack(static_cast<int>(self.type), self.id, self.name);
    }
};

template <vb::msgpack::is_packable TYPE, std::uint8_t... DATA>
TYPE test_unpack()
{
    auto [result, _] = vb::msgpack::read<TYPE>(std::array{std::byte{DATA}...});
    return result.value_or(TYPE{});
}

std::ostream& operator<<(std::ostream& out, const std::ranges::viewable_range auto& range)
requires( !requires{ range.begin()->first; } && !std::same_as<decltype(range), const std::string&>)
{
    bool first = true;
    out << "[ ";
    for (auto value: range) {
        if (first) {
            first = !first;
        } else {
            out << ", ";
        }
        out << value;
    }
    out << " ]";
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::ranges::viewable_range auto& map)
requires( requires{ map.begin()->first; })
{
    bool first = true;
    out << "{ ";
    for (auto [key, value] : map) {
        if (first) {
            first = !first;
        } else {
            out << ", ";
        }
        out << key << ": "<< value;
    }
    out << " }";
    return out;
}

int main(int, const char **)
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    std::cout << "string : "
            << test_unpack<std::string, 0xa3, 'A', 'b', 'C'>()
            << "\n";

    std::cout << "int : " << test_unpack<int, 0xcd, 0x00, 0x01>() << "\n";
//  3F 80 00 00
    std::cout
        << "float : "
        << test_unpack<float, 0xca, 0x3f, 0x80, 0x00, 0x00>()
        << "\n";

    std::cout << "array : "
              << test_unpack<std::array<int, 3>, 0x93, 0xff, 2, 3>()
              << "\n";

    std::cout << "vector : "
              << test_unpack<std::vector<std::int16_t>, 0x98, 0xff, 0xfe, 0xfd, 4, 5, 6, 7, 8>()
              << "\n";

    std::cout << "Map : "
              << test_unpack<std::map<std::string, int>,
                             0x83,
                             0xa1, 'a',
                             0x01,
                             0xa2, 'a', 'b',
                             0x02,
                             0xa3, 'a', 'b', 'c',
                             0x03>()
              << "\n";

    std::cout << "String with size : "
              << test_unpack<std::string,
        0xd9, 4, 't', 'e', 's', 't'>() << '\n';

    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}
