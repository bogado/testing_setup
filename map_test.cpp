#include <map>
#include <utility>
#include <string>
#include <ranges>
#include <algorithm>

std::map<std::string, int> m;
int v{0};

int main() {
    auto inserter = std::inserter(m, m.end());
    *inserter = std::pair{std::string{"test"}, 0};
    auto b = std::pair<const std::string, int>{"test 2", 1};
    *inserter = b;
    std::ranges::generate_n(inserter,3,[]() -> std::pair<const std::string, int>{
        auto val = std::pair{std::to_string(v), v};
        v++;
        return val;
    });
}
