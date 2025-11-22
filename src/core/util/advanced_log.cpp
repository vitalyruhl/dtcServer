#include "coinbase_dtc_core/core/util/advanced_log.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace open_dtc_server
{
    namespace util
    {

        // Static instance
        Logger &Logger::getInstance()
        {
            static Logger instance;
            return instance;
        }

        Logger::~Logger()
        {
            shutdown();
        }

        bool Logger::initialize(const std::string &config_file_path)
        {
            std::lock_guard<std::mutex> lock(log_mutex_);

            if (initialized_)
            {
                return true;
            }

            // Load configuration
            if (!load_config(config_file_path))
            {
                // Use default config if loading fails
                std::cerr << "[LOGGER] Warning: Could not load config file '" << config_file_path
                          << "', using defaults" << std::endl;
            }

            // Set log level based on profile
            switch (config_.profile)
            {
            case LogProfile::STD:
                current_level_ = LogLevel::ERROR;
                break;
            case LogProfile::ADVANCED:
                current_level_ = LogLevel::INFO;
                break;
            case LogProfile::VERBOSE:
                current_level_ = LogLevel::TRACE;
                break;
            }

            // Setup logging infrastructure
            setup_log_directory();

            // Rotate logs if enabled
            if (config_.rotate_on_startup)
            {
                rotate_logs();
            }

            // Open log file
            if (config_.file_output)
            {
                open_log_file();
            }

            initialized_ = true;

            // Log initialization
            info("Logger initialized successfully");
            info("Log Profile: " + std::to_string(static_cast<int>(config_.profile)) +
                 " (0=std, 1=advanced, 2=verbose)");
            info("Log Level: " + level_to_string(current_level_));
            info("Log File: " + config_.log_file_path);

            return true;
        }

        void Logger::setLogProfile(LogProfile profile)
        {
            std::lock_guard<std::mutex> lock(log_mutex_);
            config_.profile = profile;

            // Update log level
            switch (profile)
            {
            case LogProfile::STD:
                current_level_ = LogLevel::ERROR;
                break;
            case LogProfile::ADVANCED:
                current_level_ = LogLevel::INFO;
                break;
            case LogProfile::VERBOSE:
                current_level_ = LogLevel::TRACE;
                break;
            }

            info("Log profile changed to: " + std::to_string(static_cast<int>(profile)));
        }

        void Logger::setLogLevel(LogLevel level)
        {
            std::lock_guard<std::mutex> lock(log_mutex_);
            current_level_ = level;
            info("Log level changed to: " + level_to_string(level));
        }

        void Logger::trace(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::TRACE, message, function);
        }

        void Logger::debug(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::DEBUG, message, function);
        }

        void Logger::info(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::INFO, message, function);
        }

        void Logger::warning(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::WARNING, message, function);
        }

        void Logger::error(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::ERROR, message, function);
        }

        void Logger::critical(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::CRITICAL, message, function);
        }

        void Logger::log_dtc_message(const std::string &direction, const std::string &message_type, const std::string &content)
        {
            if (!config_.log_dtc_messages)
                return;

            std::string msg = "[DTC " + direction + "] " + message_type;
            if (!content.empty())
            {
                msg += ": " + content;
            }
            debug(msg, "DTC_MESSAGE");
        }

        void Logger::log_coinbase_api(const std::string &endpoint, const std::string &method, const std::string &response)
        {
            if (!config_.log_coinbase_api)
                return;

            std::string msg = "[COINBASE API] " + method + " " + endpoint;
            if (!response.empty() && response.length() < 200)
            {
                msg += " -> " + response;
            }
            else if (!response.empty())
            {
                msg += " -> " + response.substr(0, 200) + "... (truncated)";
            }
            debug(msg, "COINBASE_API");
        }

        void Logger::log_websocket_data(const std::string &direction, const std::string &data)
        {
            if (!config_.log_websocket_data)
                return;

            std::string msg = "[WEBSOCKET " + direction + "] ";
            if (data.length() < 300)
            {
                msg += data;
            }
            else
            {
                msg += data.substr(0, 300) + "... (truncated)";
            }
            trace(msg, "WEBSOCKET");
        }

        void Logger::log_market_data(const std::string &symbol, const std::string &data)
        {
            if (!config_.log_market_data)
                return;

            debug("[MARKET DATA] " + symbol + ": " + data, "MARKET_DATA");
        }

        void Logger::log_performance(const std::string &operation, double duration_ms)
        {
            if (!config_.log_performance)
                return;

            std::ostringstream ss;
            ss << "[PERFORMANCE] " << operation << " took " << std::fixed << std::setprecision(2) << duration_ms << "ms";
            info(ss.str(), "PERFORMANCE");
        }

        void Logger::log_network_activity(const std::string &activity, const std::string &details)
        {
            if (!config_.log_network_activity)
                return;

            debug("[NETWORK] " + activity + ": " + details, "NETWORK");
        }

        void Logger::rotate_logs()
        {
            if (!std::filesystem::exists(config_.log_file_path))
            {
                return; // No existing log to rotate
            }

            // Close current log file
            if (log_file_ && log_file_->is_open())
            {
                log_file_->close();
            }

            // Generate timestamp for rotation
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            std::ostringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");

            // Create rotated filename
            std::filesystem::path log_path(config_.log_file_path);
            std::string rotated_name = log_path.stem().string() + "_" + ss.str() + log_path.extension().string();
            std::filesystem::path rotated_path = log_path.parent_path() / rotated_name;

            try
            {
                std::filesystem::rename(config_.log_file_path, rotated_path);
            }
            catch (const std::exception &e)
            {
                std::cerr << "[LOGGER] Warning: Could not rotate log file: " << e.what() << std::endl;
            }

            // Cleanup old logs
            cleanup_old_logs();
        }

        void Logger::cleanup_old_logs()
        {
            try
            {
                std::filesystem::path log_dir(config_.log_directory);
                if (!std::filesystem::exists(log_dir))
                {
                    return;
                }

                // Collect all log files
                std::vector<std::filesystem::path> log_files;
                std::filesystem::path log_path(config_.log_file_path);
                std::string log_prefix = log_path.stem().string() + "_";

                for (const auto &entry : std::filesystem::directory_iterator(log_dir))
                {
                    if (entry.is_regular_file())
                    {
                        std::string filename = entry.path().filename().string();
                        if (filename.find(log_prefix) == 0)
                        {
                            log_files.push_back(entry.path());
                        }
                    }
                }

                // Sort by modification time (newest first)
                std::sort(log_files.begin(), log_files.end(), [](const auto &a, const auto &b)
                          { return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b); });

                // Delete old logs beyond max_log_files
                if (log_files.size() > static_cast<size_t>(config_.max_log_files))
                {
                    for (size_t i = config_.max_log_files; i < log_files.size(); ++i)
                    {
                        try
                        {
                            std::filesystem::remove(log_files[i]);
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << "[LOGGER] Warning: Could not delete old log file " << log_files[i]
                                      << ": " << e.what() << std::endl;
                        }
                    }
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "[LOGGER] Warning: Error during log cleanup: " << e.what() << std::endl;
            }
        }

        void Logger::flush()
        {
            std::lock_guard<std::mutex> lock(log_mutex_);
            if (log_file_ && log_file_->is_open())
            {
                log_file_->flush();
            }
            std::cout.flush();
        }

        void Logger::shutdown()
        {
            std::lock_guard<std::mutex> lock(log_mutex_);
            if (initialized_)
            {
                info("Logger shutting down");
                close_log_file();
                initialized_ = false;
            }
        }

        // Private methods

        void Logger::log_internal(LogLevel level, const std::string &message, const std::string &function)
        {
            if (!should_log(level))
            {
                return;
            }

            std::lock_guard<std::mutex> lock(log_mutex_);

            std::string formatted_message = format_message(level, message, function);

            // Console output
            if (config_.console_output)
            {
                if (level >= LogLevel::WARNING)
                {
                    std::cerr << formatted_message << std::endl;
                }
                else
                {
                    std::cout << formatted_message << std::endl;
                }
            }

            // File output
            if (config_.file_output && log_file_ && log_file_->is_open())
            {
                *log_file_ << formatted_message << std::endl;
                // Flush periodically for important messages
                if (level >= LogLevel::ERROR)
                {
                    log_file_->flush();
                }
            }
        }

        bool Logger::should_log(LogLevel level) const
        {
            return level >= current_level_;
        }

        std::string Logger::format_message(LogLevel level, const std::string &message, const std::string &function) const
        {
            std::string formatted = config_.log_format;

            // Replace placeholders
            size_t pos = 0;
            while ((pos = formatted.find("{timestamp}", pos)) != std::string::npos)
            {
                formatted.replace(pos, 11, get_current_timestamp());
                pos += get_current_timestamp().length();
            }

            pos = 0;
            while ((pos = formatted.find("{level}", pos)) != std::string::npos)
            {
                formatted.replace(pos, 7, level_to_string(level));
                pos += level_to_string(level).length();
            }

            pos = 0;
            while ((pos = formatted.find("{message}", pos)) != std::string::npos)
            {
                formatted.replace(pos, 9, message);
                pos += message.length();
            }

            if (config_.log_function_name && !function.empty())
            {
                pos = 0;
                while ((pos = formatted.find("{function}", pos)) != std::string::npos)
                {
                    formatted.replace(pos, 10, function);
                    pos += function.length();
                }
            }

            if (config_.log_thread_id)
            {
                pos = 0;
                std::ostringstream thread_ss;
                thread_ss << std::this_thread::get_id();
                while ((pos = formatted.find("{thread_id}", pos)) != std::string::npos)
                {
                    formatted.replace(pos, 11, thread_ss.str());
                    pos += thread_ss.str().length();
                }
            }

            return formatted;
        }

        std::string Logger::level_to_string(LogLevel level) const
        {
            switch (level)
            {
            case LogLevel::TRACE:
                return "TRACE";
            case LogLevel::DEBUG:
                return "DEBUG";
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::WARNING:
                return "WARNING";
            case LogLevel::ERROR:
                return "ERROR";
            case LogLevel::CRITICAL:
                return "CRITICAL";
            default:
                return "UNKNOWN";
            }
        }

        std::string Logger::get_current_timestamp() const
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);

            std::ostringstream ss;
            ss << std::put_time(std::localtime(&time_t), config_.timestamp_format.c_str());
            return ss.str();
        }

        bool Logger::load_config(const std::string &config_file_path)
        {
            // Simple INI file parsing - in a real implementation, you might want to use a proper INI library
            std::ifstream file(config_file_path);
            if (!file.is_open())
            {
                return false;
            }

            std::string line;
            std::string current_section;

            while (std::getline(file, line))
            {
                // Trim whitespace
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t") + 1);

                // Skip empty lines and comments
                if (line.empty() || line[0] == '#' || line[0] == ';')
                {
                    continue;
                }

                // Handle sections
                if (line[0] == '[' && line.back() == ']')
                {
                    current_section = line.substr(1, line.length() - 2);
                    continue;
                }

                // Handle key-value pairs
                size_t eq_pos = line.find('=');
                if (eq_pos == std::string::npos)
                {
                    continue;
                }

                std::string key = line.substr(0, eq_pos);
                std::string value = line.substr(eq_pos + 1);

                // Trim key and value
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                // Parse configuration values
                if (current_section == "Logging")
                {
                    if (key == "LogLevel")
                    {
                        if (value == "std")
                            config_.profile = LogProfile::STD;
                        else if (value == "advanced")
                            config_.profile = LogProfile::ADVANCED;
                        else if (value == "verbose")
                            config_.profile = LogProfile::VERBOSE;
                    }
                    else if (key == "LogFilePath")
                        config_.log_file_path = value;
                    else if (key == "LogDirectory")
                        config_.log_directory = value;
                    else if (key == "MaxLogFiles")
                        config_.max_log_files = std::stoi(value);
                    else if (key == "RotateOnStartup")
                        config_.rotate_on_startup = (value == "true");
                    else if (key == "ConsoleOutput")
                        config_.console_output = (value == "true");
                    else if (key == "FileOutput")
                        config_.file_output = (value == "true");
                    else if (key == "TimestampFormat")
                        config_.timestamp_format = value;
                    else if (key == "LogFormat")
                        config_.log_format = value;
                }
                else if (current_section == "Advanced")
                {
                    if (key == "LogThreadId")
                        config_.log_thread_id = (value == "true");
                    else if (key == "LogFunctionName")
                        config_.log_function_name = (value == "true");
                    else if (key == "LogPerformance")
                        config_.log_performance = (value == "true");
                    else if (key == "LogMemoryUsage")
                        config_.log_memory_usage = (value == "true");
                    else if (key == "LogNetworkActivity")
                        config_.log_network_activity = (value == "true");
                }
                else if (current_section == "Debug")
                {
                    if (key == "LogDTCMessages")
                        config_.log_dtc_messages = (value == "true");
                    else if (key == "LogCoinbaseAPI")
                        config_.log_coinbase_api = (value == "true");
                    else if (key == "LogWebSocketData")
                        config_.log_websocket_data = (value == "true");
                    else if (key == "LogMarketData")
                        config_.log_market_data = (value == "true");
                }
            }

            return true;
        }

        void Logger::setup_log_directory()
        {
            try
            {
                std::filesystem::path log_dir(config_.log_directory);
                if (!std::filesystem::exists(log_dir))
                {
                    std::filesystem::create_directories(log_dir);
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "[LOGGER] Warning: Could not create log directory: " << e.what() << std::endl;
            }
        }

        void Logger::open_log_file()
        {
            try
            {
                log_file_ = std::make_unique<std::ofstream>(config_.log_file_path, std::ios::app);
                if (!log_file_->is_open())
                {
                    std::cerr << "[LOGGER] Error: Could not open log file: " << config_.log_file_path << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "[LOGGER] Error opening log file: " << e.what() << std::endl;
            }
        }

        void Logger::close_log_file()
        {
            if (log_file_ && log_file_->is_open())
            {
                log_file_->close();
            }
            log_file_.reset();
        }

    } // namespace util
} // namespace open_dtc_server