#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <chrono>

namespace vb::msgpack {

enum class format_type : std::uint8_t
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

enum class format_class : std::uint8_t {
    INTEGER,
    BOOLEAN,
    FLOAT,
    ARRAY,
    MAP,
    STR,
    EXT
};

template <format_class CLASS, format_type TYPE>
constexpr bool is = []() {
    using enum format_type;
    using enum format_class;
    constexpr auto value = std::to_underlying(TYPE);
    switch (CLASS) {
    case INTEGER:
        return TYPE == POSITIVE_FIX_INT || TYPE == NEGATIVE_FIX_INT || (value >= std::to_underlying(UINT_8) && value <= std::to_underlying(INT_64));
    case BOOLEAN:
        return TYPE == FALSE || TYPE == TRUE;
    case FLOAT:
        return TYPE == FLOAT_32 || TYPE == FLOAT_64;
    case ARRAY:
        return TYPE == FIX_ARRAY || TYPE == ARRAY_16 || TYPE == ARRAY_32;
    case MAP:
        return TYPE == FIX_MAP || TYPE == MAP_16 || TYPE == MAP_32;
    case STR:
        return TYPE == FIX_STR || TYPE == STR_8 || TYPE == STR_16 || TYPE == STR_32;
    case EXT:
        return value >= std::to_underlying(FIXEXT_1) && value <= std::to_underlying(FIXEXT_16);
    }
}();

template<format_type TYPE>
constexpr bool is_fixed =
  TYPE == format_type::POSITIVE_FIX_INT ||
  TYPE == format_type::NEGATIVE_FIX_INT || TYPE == format_type::FIX_ARRAY ||
  TYPE == format_type::FIX_MAP || TYPE == format_type::FIX_STR ||
  is<format_class::EXT, TYPE>;

template <typename T>
constexpr auto format_class_of = format_class::EXT;

template <std::integral T>
constexpr auto format_class_of<T> = format_class::INTEGER;

template <std::floating_point T>
constexpr auto format_class_of<T> = format_class::FLOAT;

template <typename... Ts>
constexpr auto format_class_of<std::tuple<Ts...>> = format_class::ARRAY;

template <typename T, std::size_t SIZE>
constexpr auto format_class_of<std::array<T, SIZE>> = format_class::ARRAY;

template <typename KEY_T, typename VALUE_T>
constexpr auto format_class_of<std::map<KEY_T, VALUE_T>> = format_class::MAP;

template <typename KEY_T, typename VALUE_T>
constexpr auto format_class_of<std::unordered_map<KEY_T, VALUE_T>> = format_class::MAP;

template <typename CLOCK>
constexpr auto format_class_of<std::chrono::time_point<CLOCK>> = format_class::EXT;

template <>
constexpr auto format_class_of<std::string> = format_class::STR;

template <>
constexpr auto format_class_of<std::string_view> = format_class::STR;

format_type operator++(format_type ft) {
    return static_cast<format_type>(std::to_underlying(ft) + 1);
}

struct bounds {
    std::uint8_t type_id;
    std::uint8_t extent = 0;
    std::int8_t min = 0;
    std::int8_t max = 0;
    std::uint8_t size = 0;
};

static constexpr auto& boundaries() {
    static constexpr auto result = std::array{ bounds{ .type_id = 0x00, .extent = 0x7F , .min = 0, .max = 0x7F },
        bounds{ .type_id = 0x80, .extent = 0x0F },
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
        bounds{ .type_id = 0xD8, .size = 16},
        bounds{ .type_id = 0xD9, .size = 1 },
        bounds{ .type_id = 0xDA, .size = 2 },
        bounds{ .type_id = 0xDB, .size = 4 },
        bounds{ .type_id = 0xDC, .size = 1 },
        bounds{ .type_id = 0xDD, .size = 2 },
        bounds{ .type_id = 0xDE, .size = 1 },
        bounds{ .type_id = 0xDF, .size = 2 },
        bounds{ .type_id = 0xE0, .extent = 0x1F , .min =-0x20, .max=-1} };
    return result;
}

static constexpr bounds bounds_of(format_type t) {
    return boundaries()[std::to_underlying(t)]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

struct format_set {
    format_type fixed;
    std::uint8_t extent;
    std::array<format_type, 4> non_fixed{format_type::UNUSED, format_type::UNUSED, format_type::UNUSED, format_type::UNUSED};

    constexpr auto bounds_of(std::integral auto size) const {
        if (size < extent) {
            return std::to_underlying(fixed) + size;
        }

        for (auto [format, max] :
             non_fixed |
               std::views::transform([size = 0x1](auto value) mutable {
                   size <<= 8;
                   return std::pair{ value, size };
               })) {
            if (size < max) {
                return bounds_for(format);
            }
        }
        return bounds_for(format_type::UNUSED);
    }

private:
  template<std::same_as<format_type>... TYPES>
  constexpr format_set(format_type fix, std::uint8_t xtnt, TYPES... non_fixed_formats )
    : fixed{ fix }
    , extent{ xtnt }
  {
      std::ranges::copy(std::array{non_fixed_formats ...}, non_fixed.begin());
  }

};

constexpr static format_type type_of(std::uint8_t data) {
    auto index = static_cast<std::uint8_t>(
     std::distance(boundaries().begin(),
         std::ranges::find_if(boundaries(), [data](auto boundary) {
             return data >= boundary.type_id &&
                 (data - boundary.type_id) <= boundary.extent;
         })));
    return format_type{index};
}

static constexpr auto value_of(std::uint8_t data) 
-> std::optional<std::int8_t> {
    switch (type_of(data)){
    case format_type::POSITIVE_FIX_INT:
        return data;
    case format_type::NEGATIVE_FIX_INT:
        return static_cast<int8_t>(-0x100 + data);
    default:
        return {};
    }
}

constexpr auto format_for (const std::integral auto value) { 
    for (const auto& bound : { bounds_of(format_type::POSITIVE_FIX_INT), bounds_of(format_type::NEGATIVE_FIX_INT) }) {
        if (value >= bound.min && value <= bound.max) {
            return bounds{.type_id =  static_cast<uint8_t>(value)};
        }
    }
    constexpr auto is_signed = std::is_signed_v<decltype(value)>;

    switch(sizeof(value)) {
    case 1:
        return is_signed?bounds_of(format_type::INT_8):bounds_of(format_type::UINT_8);
    case 2:
        return is_signed?bounds_of(format_type::INT_16):bounds_of(format_type::UINT_16);
    case 4:
        return is_signed?bounds_of(format_type::INT_32):bounds_of(format_type::UINT_32);
    case 8:
        return is_signed?bounds_of(format_type::INT_64):bounds_of(format_type::UINT_64);
    }
}

template <std::floating_point FLOAT_T>
constexpr auto format_for(const FLOAT_T) {
    if constexpr (std::same_as<FLOAT_T, float>) {
        return bounds_of(format_type::FLOAT_32);
    } else {
        return bounds_of(format_type::FLOAT_64);
    }
}

constexpr auto format_for(std::string_view str)
{
    auto bnd = bounds_of(format_type::FIX_STR);
    if (str.size() <= bnd.size) {
      bnd.type_id += str.size();
      bnd.size = str.size();
    } else {
      for (auto tp : {format_type::STR_8, format_type::STR_16, format_type::STR_32}) {
        bnd = bounds_of(tp);
        if (str.size() < bnd.size) {
          break;
        }
      }
    }
    return bnd;
}

template <std::size_t SIZE>
constexpr auto format_for_array = []() {
    if constexpr(auto bound = bounds_of(format_type::FIX_ARRAY); SIZE <= bound.extent) {
        return bounds{ bound.type_id + SIZE };
    }
    if constexpr(SIZE <= std::numeric_limits<std::uint16_t>::max()) {
        return bounds_of(format_type::ARRAY_16);
    }
    return bounds_of(format_type::ARRAY_32);
}();

template <typename ARG_T, std::size_t SIZE>
constexpr auto format_for(std::array<ARG_T, SIZE>) {
    return format_for_array<SIZE>;
}

template <typename... ARGs>
constexpr auto format_for(std::tuple<ARGs...>) {
    return format_for_array<sizeof...(ARGs)>;
}

template <std::ranges::range RANGE_T>
constexpr auto format_for(const RANGE_T& range)
{
    if constexpr(
}

template <typename T>
auto dump_data(const T& value)
{
   return std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(value);
}

static_assert(format_for(10).type_id == 10);

template <typename PACKER>
concept can_pack = std::output_iterator<PACKER, std::uint8_t>;

template <typename PACKER, typename ARG>
concept is_single_packer_for =
    std::invocable<PACKER, ARG> &&
    std::same_as<std::invoke_result_t<PACKER, ARG>, std::size_t>;

template <typename PACKER, typename... ARGs>
concept is_packer_for = 
    std::invocable<PACKER, ARGs...>
    && std::same_as<std::invoke_result_t<PACKER, ARGs...>, std::size_t> &&
    (is_single_packer_for<PACKER, ARGs> && ...)
    && sizeof...(ARGs) > 0;

template <typename UNPACKER, typename ARG>
concept is_unpacker_for = std::invocable<UNPACKER>
    && std::same_as<std::invoke_result_t<UNPACKER>, ARG>;

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

    void operator++(int) {
        data.push_back(0);
    }

    uint8_t& operator*() {
        if (data.empty()) { 
            (*this)++;
        }
        return data.front();
    }

    template <std::same_as<msgpack> SELF_T>
    auto begin(this SELF_T&& self) {
        return std::forward<SELF_T>(self).data.begin();
    }

    template <std::same_as<msgpack> SELF_T>
    auto end(this SELF_T&& self) {
        return std::forward<SELF_T>(self).data.end();
    }

    bool operator()(const auto value)
    {
        auto bound = format_for(value);
        data.push_back(bound.type_id);
        std::ranges::copy(dump_data(value), std::back_inserter(data));
    }
};

static_assert(type_of(10) == format_type::POSITIVE_FIX_INT);
static_assert(type_of(0xf9) == format_type::NEGATIVE_FIX_INT);     // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(type_of(0xA0) == format_type::FIX_STR);              // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(type_of(0xAF) == format_type::FIX_STR);              // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(type_of(0xB3) == format_type::FIX_STR);              // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(30) == 30);                            // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(0xFF).has_value());                    // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(0xE0).has_value());                    // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(0xFE) == -2);                          // NOLINT(cppcoreguidelines-avoid-magic-numbers)
static_assert(value_of(0xFF) == -1);                          // NOLINT(cppcoreguidelines-avoid-magic-numbers)

}
