#include "msgpack/byte_view.hpp"

#include <cstddef>
#include <iomanip>
#include <ios>
#include <iterator>
#include <type_traits>
#include <ranges>
#include <iostream>
#include <utility>
#include <format>
#include <string>

#include "msgpack/byte_view.hpp"

auto print(const vb::is_basic_type auto arg) {
        auto bytes = vb::byte_view(arg);
        std::ranges::copy(bytes | std::views::transform([](const auto& v) { return std::format("{:#04x}", std::to_underlying(v)); }), std::ostream_iterator<std::string>(std::cout, " "));
        std::cout << "\n";
}

int main(int argc, const char * argv[])
{
    for (auto arg : std::span(argv, argc) | std::views::drop(1) | std::views::transform([](auto a) {return std::string(a); })) {
        if (arg.contains('.')) 
        {
            print(std::stod(arg));
        } else {
            print(std::stoll(arg));
        }
    }
}
