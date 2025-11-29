#include "coinbase_dtc_core/core/util/advanced_log.hpp"
#include <exception>
#include <iostream>

int main()
{
    std::cout << "[INFO] Logger simple creation test starting" << std::endl;

    try
    {
        auto &logger = open_dtc_server::util::Logger::getInstance();
        (void)logger;
        std::cout << "[SUCCESS] Logger instance retrieved successfully" << std::endl;
        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cout << "[ERROR] Exception while creating logger: " << ex.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << "[ERROR] Unknown exception while creating logger" << std::endl;
        return 2;
    }
}
