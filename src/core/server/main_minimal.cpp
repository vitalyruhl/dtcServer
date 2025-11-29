#include <iostream>
#include <string>
#include <exception>

// Minimal test to check if the problem is in our includes or basic setup
int main(int argc, char *argv[])
{
    std::cout << "=== Minimal DTC Server Test ===" << std::endl;
    std::cout << "Arguments received: " << argc << std::endl;

    for (int i = 0; i < argc; ++i)
    {
        std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
    }

    try
    {
        std::cout << "Testing basic C++ functionality..." << std::endl;
        std::string test = "Hello World";
        std::cout << "String test: " << test << std::endl;

        std::cout << "Basic test completed successfully" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << "Unknown exception caught" << std::endl;
        return 2;
    }
}