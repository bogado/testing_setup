#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <cctype>
#include <print>

#include "./msgpack.hpp"

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
/*
template <typename ... ARGs>
auto request(std::string_view name, ARGs... args)
{
    return vb::packer{}(payload{type::REQUEST, std::string{name}, std::move(args)...});
}

*/

int main(int, const char **)
{
    using namespace vb::msgpack;
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    std::string value;
    unpack(
      std::array{
        std::byte{ 0xa3 }, std::byte{ 65 }, std::byte{ 66 }, std::byte{ 67 } },
      value);
    std::cout << "string : " << value << "\n";
    std::size_t int_value{ 2 };
    unpack(
      std::array{ std::byte{ 0xcd }, std::byte{ 0x00 }, std::byte{ 0x01 } },
      int_value);
    std::cout << "int : " << int_value << "\n";
    std::array<int, 2> array_value{1,2};
    unpack(std::array{ std::byte{ 0x92 }, std::byte{ 0x1 }, std::byte{ 0xff } },
           array_value);
    std::print("Array value : ");
    std::ranges::copy(array_value, std::ostream_iterator<int>(std::cout, ", "));
    std::println();

    std::map<std::string, int> map_val{};
    unpack(std::array{ std::byte{ 0x83 },
        std::byte{0xa1}, std::byte{'a'}, 
        std::byte{ 0x1 },
        std::byte{0xa2}, std::byte{'a'}, std::byte{'b'},
        std::byte{ 0x2 },
        std::byte{0xa3}, std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, 
        std::byte{ 0x3 }
    }, map_val);
    std::print("Map value : {{");
    auto first = true;
    for (auto [key, value] : map_val) {
        if (first) {
            first = false;
        } else {
            std::cout << ",";
        }
        std::cout << " '" << key << "': " << value;
    }
    std::cout << "} \n";
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}
