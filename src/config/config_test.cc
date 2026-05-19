#include "config.h"

#include <toml++/toml.hpp>

#include <iostream>

using namespace std::literals;


template<class StringType>
void do_something_with_string_values(const StringType& value);

int main(int argc, char const *argv[])
{

    auto config = toml::parse_file( "config.toml" );
    
    // int port = config["server"]["port"].value_or(8070);

    std::cout << config << std::endl;

    auto configuration = Config::LoadSystemConfig();

    std::cout << configuration << std::endl;


    return 0;
}

template<class StringType>
void do_something_with_string_values(const StringType& value) {
    std::cout << value << std::endl;
}