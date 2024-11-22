#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace vb::msgpack {

enum class type : std::uint8_t
{
    POSITIVE_FIX_INT,
    FIX_MAP,
    FIX_ARRAY,
    FIX_STR,
    NIL,
    UNUSED,
    FALSE,
    TRUE,
    BIN_8,
    BIN_16,
    BIN_32,
    EXT_8,
    EXT_16,
    EXT_32,
    FLOAT_32,
    FLOAT_64,
    UINT_8,
    UINT_16,
    UINT_32,
    UINT_64,
    INT_8,
    INT_16,
    INT_32,
    INT_64,
    FIXEXT_1,
    FIXEXT_2,
    FIXEXT_4,
    FIXEXT_8,
    FIXEXT_16,
    STR_8,
    STR_16,
    STR_32,
    ARRAY_16,
    ARRAY_32,
    MAP_16,
    MAP_32,
    NEGATIVE_FIX_INT
};

struct bounds {
    std::uint8_t type_id;
    std::uint8_t extent = 0;
    std::int8_t min = 0;
    std::int8_t max = 0;
    std::uint8_t size = 0;
};

static constexpr auto& boundaries() {
    static constexpr auto result = std::array{ bounds{ .type_id = 0x00, .extent = 0x7F },
        bounds{ .type_id = 0x80, .extent = 0x0F, .min = 0, .max = 0xF },
        bounds{ .type_id = 0x90, .extent = 0x0F },
        bounds{ .type_id = 0xA0, .extent = 0x1F },
        bounds{ .type_id = 0xC0 },
        bounds{ .type_id = 0xC1 },
        bounds{ .type_id = 0xC2 },
        bounds{ .type_id = 0xC3 },
        bounds{ .type_id = 0xC4, .size = 1 },
        bounds{ .type_id = 0xC5, .size = 2 },
        bounds{ .type_id = 0xC6, .size = 4 },
        bounds{ .type_id = 0xC7, .size = 1 },
        bounds{ .type_id = 0xC8, .size = 2 },
        bounds{ .type_id = 0xC9, .size = 4 },
        bounds{ .type_id = 0xCA, .size = 2 },
        bounds{ .type_id = 0xCB, .size = 4 },
        bounds{ .type_id = 0xCC, .size = 1 },
        bounds{ .type_id = 0xCD, .size = 2 },
        bounds{ .type_id = 0xCE, .size = 4 },
        bounds{ .type_id = 0xCF, .size = 8 },
        bounds{ .type_id = 0xD0, .size = 1 },
        bounds{ .type_id = 0xD1, .size = 2 },
        bounds{ .type_id = 0xD2, .size = 4 },
        bounds{ .type_id = 0xD3, .size = 8 },
        bounds{ .type_id = 0xD4, .size = 1 },
        bounds{ .type_id = 0xD5, .size = 2 },
        bounds{ .type_id = 0xD6, .size = 4 },
        bounds{ .type_id = 0xD7, .size = 8 },
        bounds{ .type_id = 0xD8, .size = 16 },
        bounds{ .type_id = 0xD9, .size = 1  },
        bounds{ .type_id = 0xDA, .size = 2  },
        bounds{ .type_id = 0xDB, .size = 4  },
        bounds{ .type_id = 0xDC, .size = 1 },
        bounds{ .type_id = 0xDD, .size = 2 },
        bounds{ .type_id = 0xDE, .size = 1 },
        bounds{ .type_id = 0xDF, .size = 2 },
        bounds{ .type_id = 0xE0, .extent = 0x1F } };
    return result;
}

static constexpr bounds bounds_of(type t) {
    return boundaries()[std::to_underlying(t)]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

constexpr static type type_of(std::uint8_t data) {
    auto index = static_cast<std::uint8_t>(
     std::distance(boundaries().begin(),
         std::ranges::find_if(boundaries(), [data](auto boundary) {
             return data >= boundary.type_id &&
                 (data - boundary.type_id) <= boundary.extent;
         })));
    return type{index};
}

static constexpr auto value_of(std::uint8_t data) 
-> std::optional<std::int8_t>{
    switch (type_of(data)){
    case type::POSITIVE_FIX_INT:
        return data;
    case type::NEGATIVE_FIX_INT:
        return static_cast<int8_t>(-0x100 + data);
    default:
        return {};
    }
}

constexpr auto format_for (const std::integral auto value) { 
    for (auto bound : { bounds_of(type::POSITIVE_FIX_INT), bounds_of(type::NEGATIVE_FIX_INT) }) {
        if (value >= bound.min && value <= bound.max) {
            return bounds{static_cast<uint8_t>(value)};
        }
    }
    constexpr auto is_signed = std::is_signed_v<decltype(value)>;

    switch(sizeof(value)) {
    case 1:
        return is_signed?bounds_of(type::INT_8):bounds_of(type::UINT_8);
    case 2:
        return is_signed?bounds_of(type::INT_16):bounds_of(type::UINT_16);
    case 4:
        return is_signed?bounds_of(type::INT_32):bounds_of(type::UINT_32);
    case 8:
        return is_signed?bounds_of(type::INT_64):bounds_of(type::UINT_64);
    }
}

static_assert(format_for(10).type_id == 10);

struct msgpack
{
    std::vector<uint8_t> data;

    template<typename... ARG_Ts>
        requires(sizeof...(ARG_Ts) >= 2)
    bool operator()(const ARG_Ts&...args)
    {
        (*this(args) && ...);
        return true;
    }

    bool operator()(std::integral auto value)
    {
        auto [bound, fixed] = format_for(value);
        data.push_back(bound.start);
        if (!fixed) {
            for (auto _: std::ranges::iota_view(bound.size)) {
                data.push_back(value % 0x100);
                value /= 0x100;
            }
        }
    }
};

static_assert(type_of(10) == type::POSITIVE_FIX_INT);
static_assert(type_of(0xf9) == type::NEGATIVE_FIX_INT);     // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(type_of(0xA0) == type::FIX_STR);              // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(type_of(0xAF) == type::FIX_STR);              // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(type_of(0xB3) == type::FIX_STR);              // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(30) == 30);                            // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(0xFF).has_value());                    // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(0xE0).has_value());                    // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(0xFE) == -2);                          // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(0xFF) == -1);                          // NOLINT(cppcoreguidelines-avoid-magic-numbers)

}
