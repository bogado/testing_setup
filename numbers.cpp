#include <algorithm>
#include <print>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>
#include <concepts>

auto bit_view(std::ranges::random_access_range auto data)
{
    return std::ranges::iota_view(std::size_t{0}, std::size(data)*8) | std::views::transform([data](unsigned bit_pos) {
        auto index = static_cast<std::size_t>(bit_pos / 8);
        auto bit = bit_pos % 8;
        return std::pair(bit_pos, ((1 << bit) bitand data.at(index)) != 0);
    });
}

template <std::integral INT_T>
auto get_bin_view(INT_T val)
{
    using data_t = std::array<char, sizeof(INT_T)>;
    // This is for getting a proper bit representation.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return bit_view(std::bit_cast<data_t>(val)); 
}

void print_bin(std::integral auto val)
{
    std::array<std::string, 3> headers;
    std::string bit_repr;
    for (auto [pos, bit]: get_bin_view(val)) {
        headers[0].push_back(
             (pos%10 == 0)?static_cast<char>('0' + pos/10):(pos%5 == 0)?'+':'.');
        headers[1].push_back(static_cast<char>('0' + pos%10));
        headers[2].push_back('-');
        bit_repr.push_back(bit?'1':'0');
    }
    for (auto header : headers) {
        std::ranges::reverse(header);
        std::println("{}", header);
    }
    std::ranges::reverse(bit_repr);
    std::println("{}", bit_repr);
}

auto stats_add(std::integral auto value, std::ranges::range auto bit_count)
{
    for (auto [pos, ch]: get_bin_view(value)) {
        if(ch) {
            bit_count.at(pos)++;
        }
    }
    return bit_count;
}

int main(int argc, const char** argv)
{
    // Arguments are C arrays.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::vector<std::string> args{argv, argv+argc};

    if (args.size() < 2) {
        std::println("Usage {} <number>...", args[0]);
        return 1;
    }
    std::array<uint, 128> bit_count{};
    std::ranges::fill(bit_count, 0);

    for (auto value : 
         args | std::views::drop(1) |
           std::views::transform([](auto v) { return std::stoull(v, nullptr, 0); })) {
        bit_count = stats_add(value, bit_count);
        std::println("");
        std::println("{:d}: Hex {:x} Oct {:o}", value, value, value);
        print_bin(value);
    }

    std::println("");
    std::println("Stats:");
    auto counts = std::ranges::zip_view(std::views::iota(0), bit_count) | std::views::filter([&](auto kv) {
        auto [bit, count] = kv;
        return count > args.size()/2;
    }) |  std::ranges::to<std::vector>();

    std::ranges::sort(counts, [](auto a, auto b) {
        return std::get<1>(a) > std::get<1>(b);
    });

    for(auto [bit, count] : counts) {
        std::println("bit {} set {:+5.1f}%", bit, (100.0*bit_count.at(bit))/(args.size()-1));
    }
    
    return 0;
}
