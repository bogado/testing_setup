#ifndef INCLUDED_READER_HPP
#define INCLUDED_READER_HPP

#include "types.hpp"
#include "format_type.hpp"
#include "format.hpp"

#include <algorithm>
#include <concepts>
#include <numeric>
#include <ranges>
#include <source_location>
#include <stdexcept>
#include <string>
#include <iterator>

namespace vb::msgpack {

using namespace std::literals;
struct read_failure : std::logic_error {

    read_failure(std::string error, std::source_location location = std::source_location::current()) :
        std::logic_error{"Read failure " + error + " : «"s + location.function_name() + "» "s + std::string{location.file_name()} + ":"s + std::to_string(location.line())}
    {}
};

template<is_packing_source SOURCE_T>
struct read_context {
    using source_type = SOURCE_T;
    using value_type = std::ranges::range_value_t<source_type>;
    using iterator = source_type::const_iterator;
    using const_iterator = source_type::const_iterator;
    using sentinel = source_type::const_iterator;
    using const_sentinel = source_type::const_iterator;

    iterator position;
    sentinel end_position;

    constexpr read_context(iterator pos, sentinel end_pos) :
        position{pos},
        end_position{end_pos}
    {}

    explicit constexpr read_context(const source_type& source) :
        position{std::ranges::cbegin(source)},
        end_position{std::ranges::cend(source)}
    {}

    constexpr iterator begin() {
        return position;
    }

    constexpr sentinel end() {
        return end_position;
    }

    constexpr auto front() const {
        if (position == end_position) {
            return value_type{};
        }
        return *position;
    }

    constexpr read_context advance(std::size_t size) const {
        return {std::next(position, size), end_position};
    }
};

static_assert(is_packing_source<read_context<std::array<std::byte, 5>>>);

template<is_packable TYPE, is_packing_source SOURCE_T>
struct read_result
{
    using value_type  = TYPE;
    using source_type = read_context<SOURCE_T>;

    struct invalid_result : std::logic_error
    {
        explicit invalid_result(std::source_location location)
          : std::logic_error{std::string{"No value was retrieved from the source at "s + location.function_name() + " at " + location.file_name() + ":" + std::to_string(location.line())}}
        {
        }
    };

    constexpr read_result(value_type value, source_type previous)
      : result{ value }
      , context{ previous }
    {
    }

    constexpr value_type value(std::source_location location = std::source_location::current()) const
    {
        if (!result.has_value()) {
            throw invalid_result(location);
        }
        return result.value();
    }

    std::optional<value_type> result;
    source_type               context;
};

template <typename T, std::size_t SIZE>
auto make_insert_iterator(std::array<T, SIZE>& array)
{
    return std::begin(array);
}

template <is_map TYPE>
auto make_insert_iterator(TYPE& map)
{
    return std::inserter(map, std::begin(map));
}

template <std::ranges::range TYPE>
requires(!is_map<TYPE>)
auto make_insert_iterator(TYPE& map)
{
    return std::back_inserter(map);
}

template<std::ranges::range RANGE_T, is_packing_source SOURCE_T>
requires(std::default_initializable<RANGE_T>)
constexpr read_result<RANGE_T, SOURCE_T> read_all(read_context<SOURCE_T> source, std::size_t size)
{
    if constexpr (std::same_as<RANGE_T, std::string>) {
        auto str = source | std::views::transform([](std::byte val) {
                       return static_cast<char>(val);
                   }) | std::views::take(size);
        return { std::string{ std::begin(str), std::end(str) }, source.advance(size) };
    } else {
        RANGE_T result{};
        if constexpr (requires {
                          { result.reserve(size) };
                      }) {
            result.reserve(size);
        }

        using value_t = std::ranges::range_value_t<RANGE_T>;
        auto inserter = make_insert_iterator(result);
        std::ranges::copy(std::ranges::iota_view(size) | std::views::transform([&source](auto _) mutable {
            if constexpr (is_map<RANGE_T>) {
                auto key_result = read<typename RANGE_T::key_type>(source);
                auto mapped_result = read<typename RANGE_T::mapped_type>(key_result.context);
                source = mapped_result.context;
                return value_t{key_result.value(), mapped_result.value()};
            } else {
                auto item_result = read<value_t>(source);
                source = item_result.context;
                return item_result.value();
            }
        }), inserter); 
        return read_result{result, source};
    }
}

template<typename TYPE, is_packing_source SOURCE_T, format::is_traits TRAITS_T>
constexpr auto 
read_value(read_context<SOURCE_T> source [[maybe_unused]], TRAITS_T traits)
  -> read_result<TYPE, SOURCE_T>
{
    if constexpr (!TRAITS_T::template accepts_type<TYPE>) {
        return { {}, source };
    } else if constexpr (TRAITS_T::is(format::classification::VALUE)) {
        return { traits.value(), source };
    } else {
        if (auto content_size = traits.content_size(); content_size > 0) {
            if constexpr (TRAITS_T::is(format::category_t::CONTAINER)) {
                return read_all<TYPE>(source, content_size);
            } else {
                return read_value<TYPE>(source, content_size);
            }
        } else if (auto count_len = traits.length_size; count_len > 0) {
            auto result = read_value<std::size_t>(source, count_len);
            if constexpr (std::ranges::range<TYPE>) {
                return read_all<TYPE>(result.context, result.value());
            } else {
                return read_value<TYPE>(result.context, result.value());
            }
        }
        throw std::logic_error("Unexpected missing value");
    }
}

template<typename VALUE_T, is_packing_source SOURCE_T>
    requires(!std::integral<VALUE_T>)
constexpr auto
read_value(read_context<SOURCE_T> source, std::size_t size) -> read_result<VALUE_T, SOURCE_T>
{
    VALUE_T result{};
    auto output = byte_view(result);
    std::ranges::copy(source | std::views::take(size), std::ranges::begin(output));
    return {result, source.advance(size)};
}

template<std::integral VALUE_T, is_packing_source SOURCE_T>
constexpr auto read_value(read_context<SOURCE_T> source, std::size_t size)
      -> read_result<VALUE_T, SOURCE_T>
{
    auto result =
      source | std::views::take(size) |
      std::views::transform([](auto byte) { return std::to_underlying(byte); });
    return read_result{std::accumulate(
      std::begin(result),
      std::end(result),
      VALUE_T{ 0 },
      [](VALUE_T value, auto next) { return (value << 8) + next; }), source.advance(size)};
}

template <is_packable TYPE, is_packing_source SOURCE_T>
constexpr auto read(read_context<SOURCE_T> source)
-> read_result<TYPE, SOURCE_T>
{
    auto traits = format::classification{source.front()};
    if (traits.accepts<TYPE>()) {
        source = source.advance(1);
        return traits.visitor([&source](auto traits) {
            return read_value<TYPE>(source, traits);
        }, read_result{TYPE{}, source});
    } else {
        throw read_failure("Type is not acceptable");
    }
}

template <is_packable TYPE, is_packing_source SOURCE_T>
read_result<TYPE, SOURCE_T> read(const SOURCE_T& source)
{
    if (std::ranges::empty(source)) {
        return {TYPE{}, read_context{source}};
    }
    return read<TYPE>(read_context(source));
}

}
#endif // INCLUDED_READER_HPP
