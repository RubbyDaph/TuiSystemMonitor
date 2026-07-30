#include "app/application.hpp"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        tsm::Application application;
        return application.Run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "tsm: " << error.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "tsm: Unknown fatal error\n";
    }

    return 1;
}
