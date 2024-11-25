#include "./all.hpp"
#include <iostream>
#include <string>
#include <cctype>
#include "./msgpack.hpp"

namespace message
{

enum class type : unsigned {
    REQUEST = 0,
    RESPONSE = 1,
    NOTIFICATION = 2
};

template <typename... ARGs>
struct payload {
    static inline auto last_id = std::uint32_t{0};

    payload(type mtype, const std::string& cmd, ARGs... args)
        : type{mtype}
        , id{last_id}
        , name{cmd}
        , arguments{std::move(args)...}
    {
        last_id++;
    }

    type type;
    std::uint32_t id;
    std::string name;
    std::tuple<ARGs...> arguments;

    void pack(this auto&& self, auto &pack) {
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
}

int main(int argc, const char **argv)
{
    std::cout << argv[0];
    return argc;
    /*
    using boost::asio::local::stream_protocol;

    if (argc < 2) {
        std::println("{} <unix socket> \n", argv[0]);
        return -1;
    }
    auto socket_path = std::string_view{argv[1]};
 
    stream_protocol::endpoint test{socket_path};
    stream_protocol::iostream io{test};

    if (!io) {
        std::cerr << "Could not connect to " << socket_path << "\n";
        std::cerr << io.error().message() << "\n";
    }

    auto test_message = message::request("nvim_eval", "\"Hello \" . \"world");
    
    while(true) {
        std::string line;
        std::getline(io, line);
        std::cout << line;
    } 
    */
}
