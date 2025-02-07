#ifndef INCLUDED_FORMAT_ID_HPP
#define INCLUDED_FORMAT_ID_HPP

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <utility>
namespace vb::msgpack::format {

struct id_type {
    std::byte ident;

    constexpr id_type() = default;

    explicit constexpr id_type(std::byte v)
        : ident{v}
    {}

    explicit constexpr id_type(std::uint8_t v)
      : ident{v}
    {}

    struct reach_t {
        std::uint8_t length;
    };

    constexpr std::uint8_t value() const
    {
        return static_cast<uint8_t>(ident);
    }

    constexpr id_type &operator +=(std::int8_t increment)
    {
        ident = static_cast<std::byte>(value() + increment);
        return *this;
    }

    constexpr id_type operator +(std::int8_t increment) const
    {
        return id_type{static_cast<std::uint8_t>(value() + increment)};
    }

    constexpr id_type operator ++(int)
    {
        id_type other = *this;
        *this += 1;
        return other;
    }

    constexpr id_type &operator ++()
    {
        *this += 1;
        return *this;
    }

    constexpr id_type operator++() const {
        return *this + 1;
    }

    constexpr id_type middle(id_type other) const {
        return id_type{static_cast<std::byte>(value() + other.value()/2)};
    }

    constexpr bool inside(std::pair<id_type, id_type> range) {
        return value() >= range.first.value() &&
               value() <= range.second.value();
    }

    template<typename VALUE_T>
        requires(sizeof(VALUE_T) == 1)
    struct directValue
    {
        using value_type = VALUE_T;
        value_type value;
    };

    template<typename VALUE_T>
    requires (sizeof(VALUE_T) == 1) 
    constexpr id_type(id_type predecessor, directValue<VALUE_T> val, VALUE_T value) :
        ident{predecessor.value() + value -
            val.value()}
    {}

    constexpr bool operator==(const id_type&) const = default;
    constexpr bool operator!=(const id_type&) const = default;

   friend std::ostream& operator <<(std::ostream& out, id_type id) {
        return out << "ID{" << std::hex << id.value() << "}";
    }
};

namespace literals {

constexpr auto operator ""_id(unsigned long long value) {
    return id_type{static_cast<uint8_t>(value)};
}

}

}
#endif // INCLUDED_FORMAT_ID_HPP

