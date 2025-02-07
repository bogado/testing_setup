#ifndef INCLUDED_TYPES_HPP
#define INCLUDED_TYPES_HPP

#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <map>
#include <ranges>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <iterator>

namespace vb::msgpack {

enum class type_t : std::uint8_t
{
    INTEGER,
    BOOL,
    FLOAT,
    STR,
    ARRAY,
    MAP,
    EXT,
    VOID,
    BIN,
    NO_TYPE
};

enum class direction 
{
    PACKING,   // From object to data
    UNPACKING  // From data to object
};

constexpr bool is_valid(type_t type)
{
    return type != type_t::NO_TYPE;
}

template <typename T1, typename ... Ts>
constexpr auto not_anyof = ((!std::same_as<T1, Ts>) && ... && true);

template <typename T, typename... Ts>
constexpr auto all_different = []() {
    if constexpr(sizeof...(Ts) > 0) {
        return not_anyof<T, Ts...> && all_different<Ts...>;
    } else {
        return true;
    }
}();

template <typename... Ts>
requires (all_different<Ts...>)
using type_set = std::variant<Ts...>;

using int_value =
  type_set<std::int8_t, std::int16_t, std::int32_t, std::int64_t>;

using unsigned_value =
  type_set<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>;

using float_value = type_set<float, double>;

template <int LEN>
constexpr unsigned bit_size = (LEN == 8 || LEN == 16 || LEN == 32 || LEN == 64)?LEN:8;

template <int LEN>
constexpr unsigned var_index = std::bit_width(bit_size<LEN>/8)-1;

template<int LEN, bool IS_SIGNED = false>
using integer_type = std::conditional_t<
  LEN == bit_size<LEN>,
  std::variant_alternative_t<
    var_index<LEN>,
    std::conditional_t<IS_SIGNED, int_value, unsigned_value>>,
  std::conditional_t<IS_SIGNED, int, unsigned>>;

static_assert(std::same_as<integer_type<64>, std::uint64_t>);
static_assert(std::same_as<integer_type<32, true>, std::int32_t>);

template<unsigned LEN>
using floating_type = std::variant_alternative_t<std::bit_width(LEN/64), float_value>;

static_assert(std::same_as<floating_type<32>, float>);

template<typename CLASS_T>
concept is_decomposable = requires(const CLASS_T val) {
    { std::tuple_size_v<CLASS_T> } -> std::unsigned_integral;
    { std::get<0>(val) };
};

template<typename TYPE>
concept is_map = requires(std::map<std::string, int> load) {
    typename TYPE::key_type;
    typename TYPE::mapped_type;
};

template <typename TYPE>
concept is_packable = is_decomposable<TYPE> || std::is_fundamental_v<TYPE> || std::ranges::range<TYPE>;

template<type_t TYPE_VAL, is_packable TYPE>
constexpr bool type_accepts = []() {
    using enum type_t;
    using enum direction;
    switch (TYPE_VAL) {
    case INTEGER:
        return std::integral<TYPE>;
    case BOOL:
        return std::same_as<TYPE, bool>;
    case FLOAT:
        return std::floating_point<TYPE>;
    case STR:
        return std::same_as<TYPE, std::string> ||
               std::same_as<TYPE, std::string_view> ||
                                   std::same_as<TYPE, const char *>;
    case ARRAY:
        if constexpr (is_decomposable<TYPE>) {
            return true;
        } else {
            return std::ranges::range<TYPE> && !is_map<TYPE>;
        }
    case MAP:
        return is_map<TYPE>;
    case BIN:
        return std::is_trivially_copyable_v<TYPE>;
    case VOID:
        return std::default_initializable<TYPE>;
    default:
        return false;
    }
}();

template<is_packable TYPE>
constexpr type_t type_of = []() {
    using enum type_t;
    for (auto [accepts, result] :
         { std::pair{ type_accepts<INTEGER, TYPE>, INTEGER },
           std::pair{ type_accepts<BOOL, TYPE>, BOOL },
           std::pair{ type_accepts<FLOAT, TYPE>, FLOAT },
           std::pair{ type_accepts<STR, TYPE>, STR },
           std::pair{ type_accepts<ARRAY, TYPE>, ARRAY },
           std::pair{ type_accepts<MAP, TYPE>, MAP },
           std::pair{ type_accepts<EXT, TYPE>, EXT },
           std::pair{ type_accepts<VOID, TYPE>, VOID },
           std::pair{ type_accepts<BIN, TYPE>, BIN } }) {
        if (accepts) {
            return result;
        }
    }
    return NO_TYPE;
}();

template <typename TARGET>
concept is_packing_target = std::output_iterator<TARGET, std::byte>;

template <typename SOURCE>
concept is_packing_source = std::ranges::range<SOURCE> && std::same_as<std::ranges::range_value_t<SOURCE>, std::byte>;
static_assert(type_accepts<type_t::STR, std::string>);
static_assert(is_map<std::map<std::string, int>>);

}

#endif // INCLUDED_TYPES_HPP
