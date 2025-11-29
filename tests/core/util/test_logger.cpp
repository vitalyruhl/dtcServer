#include "coinbase_dtc_core/core/util/advanced_log.hpp"
#include <exception>
#include <iostream>

int main()
{
    std::cout << "[INFO] Logger component test starting" << std::endl;

    try
    {
        auto &logger = open_dtc_server::util::Logger::getInstance();
        if (!logger.initialize("config/logging.ini"))
        {
            std::cout << "[ERROR] Logger initialization failed" << std::endl;
            return 1;
        }

        LOG_INFO("Logger initialization smoke test message");
        const std::string log_path = logger.get_full_log_path();
        if (log_path.empty())
        {
            std::cout << "[ERROR] Logger returned empty log path" << std::endl;
            return 2;
        }

        std::cout << "[SUCCESS] Logger initialized, log path: " << log_path << std::endl;
        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cout << "[ERROR] Exception while testing logger: " << ex.what() << std::endl;
        return 3;
    }
    catch (...)
    {
        std::cout << "[ERROR] Unknown exception while testing logger" << std::endl;
        return 4;
    }
}
