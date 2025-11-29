#include "coinbase_dtc_core/core/util/advanced_log.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <regex>
#include <chrono>

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
                std::cerr << "[LOGGER] Warning: Could not load config file '" << config_file_path
                          << "', using defaults" << std::endl;
            }

            // Set log level based on profile
            switch (config_.profile)
            {
            case LogProfile::PROFILE_STD:
                current_level_ = LogLevel::LOG_ERROR;
                break;
            case LogProfile::PROFILE_ADVANCED:
                current_level_ = LogLevel::LOG_INFO;
                break;
            case LogProfile::PROFILE_VERBOSE:
                current_level_ = LogLevel::LOG_TRACE;
                break;
            }

            setup_log_directory();

            // Rotate logs on startup if configured
            if (config_.rotate_on_startup)
            {
                rotate_logs_unsafe();
            }
            else
            {
                open_log_file();
            }

            initialized_ = true;
            return true;
        }

        void Logger::setLogProfile(LogProfile profile)
        {
            config_.profile = profile;
        }

        void Logger::setLogLevel(LogLevel level)
        {
            current_level_ = level;
        }

        void Logger::trace(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::LOG_TRACE, message, function);
        }

        void Logger::debug(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::LOG_DEBUG, message, function);
        }

        void Logger::info(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::LOG_INFO, message, function);
        }

        void Logger::warn(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::LOG_WARN, message, function);
        }

        void Logger::error(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::LOG_ERROR, message, function);
        }

        void Logger::fatal(const std::string &message, const std::string &function)
        {
            log_internal(LogLevel::LOG_FATAL, message, function);
        }

        void Logger::log_dtc_message(const std::string &direction, const std::string &message_type, const std::string &content)
        {
            if (config_.log_dtc_messages)
            {
                std::string msg = "[DTC] " + direction + " " + message_type + ": " + content;
                log_internal(LogLevel::LOG_DEBUG, msg);
            }
        }

        void Logger::log_coinbase_api(const std::string &endpoint, const std::string &method, const std::string &response)
        {
            if (config_.log_coinbase_api)
            {
                std::string msg = "[API] " + method + " " + endpoint + ": " + response;
                log_internal(LogLevel::LOG_DEBUG, msg);
            }
        }

        void Logger::log_websocket_data(const std::string &direction, const std::string &data)
        {
            if (config_.log_websocket_data)
            {
                std::string msg = "[WS] " + direction + ": " + data;
                log_internal(LogLevel::LOG_TRACE, msg);
            }
        }

        void Logger::log_market_data(const std::string &symbol, const std::string &data)
        {
            if (config_.log_market_data)
            {
                std::string msg = "[MARKET] " + symbol + ": " + data;
                log_internal(LogLevel::LOG_DEBUG, msg);
            }
        }

        void Logger::log_performance(const std::string &operation, double duration_ms)
        {
            std::string msg = "[PERF] " + operation + ": " + std::to_string(duration_ms) + "ms";
            log_internal(LogLevel::LOG_INFO, msg);
        }

        void Logger::log_network_activity(const std::string &activity, const std::string &details)
        {
            std::string msg = "[NET] " + activity + ": " + details;
            log_internal(LogLevel::LOG_DEBUG, msg);
        }

        void Logger::rotate_logs()
        {
            std::lock_guard<std::mutex> lock(log_mutex_);
            rotate_logs_unsafe();
        }

        void Logger::rotate_logs_unsafe()
        {
            // Simple rotation implementation (no lock needed - caller handles it)
            close_log_file();

            std::string full_path = get_full_log_path();
            for (int i = config_.max_log_files - 1; i > 0; --i)
            {
                std::string old_file = full_path + "." + std::to_string(i);
                std::string new_file = full_path + "." + std::to_string(i + 1);
                if (std::filesystem::exists(old_file))
                {
                    std::filesystem::rename(old_file, new_file);
                }
            }

            if (std::filesystem::exists(full_path))
            {
                std::filesystem::rename(full_path, full_path + ".1");
            }

            open_log_file();
        }

        void Logger::rotate_if_size_exceeded()
        {
            if (config_.rotate_on_size && get_current_log_file_size() >= parse_size_string(config_.max_file_size))
            {
                rotate_logs();
            }
        }

        void Logger::cleanup_old_logs()
        {
            std::string full_path = get_full_log_path();
            for (int i = config_.max_log_files + 1; i <= 20; ++i)
            {
                std::string old_file = full_path + "." + std::to_string(i);
                if (std::filesystem::exists(old_file))
                {
                    std::filesystem::remove(old_file);
                }
            }
        }

        size_t Logger::parse_size_string(const std::string &size_str) const
        {
            std::regex size_regex(R"((\d+)\s*(B|KB|MB|GB)?)");
            std::smatch matches;

            if (std::regex_match(size_str, matches, size_regex))
            {
                size_t value = std::stoull(matches[1].str());
                std::string unit = matches[2].str();

                if (unit.empty() || unit == "B")
                    return value;
                else if (unit == "KB")
                    return value * 1024;
                else if (unit == "MB")
                    return value * 1024 * 1024;
                else if (unit == "GB")
                    return value * 1024 * 1024 * 1024;
            }

            return 10 * 1024 * 1024; // Default 10MB
        }

        size_t Logger::get_current_log_file_size() const
        {
            std::string full_path = get_full_log_path();
            if (std::filesystem::exists(full_path))
            {
                return std::filesystem::file_size(full_path);
            }
            return 0;
        }

        std::string Logger::get_full_log_path() const
        {
            return config_.log_directory + "/" + config_.log_file_name;
        }

        void Logger::flush()
        {
            std::lock_guard<std::mutex> lock(log_mutex_);
            if (log_file_ && log_file_->is_open())
            {
                log_file_->flush();
            }
        }

        void Logger::shutdown()
        {
            std::lock_guard<std::mutex> lock(log_mutex_);
            close_log_file();
            initialized_ = false;
        }

        void Logger::log_internal(LogLevel level, const std::string &message, const std::string &function)
        {
            if (!should_log(level))
            {
                return;
            }

            std::lock_guard<std::mutex> lock(log_mutex_);
            log_internal_unsafe(level, message, function);
        }
        void Logger::log_internal_unsafe(LogLevel level, const std::string &message, const std::string &function)
        {
            // Check if we need to rotate (only after initialization)
            if (initialized_)
            {
                const_cast<Logger *>(this)->rotate_if_size_exceeded();
            }

            std::string formatted = format_message(level, message, function);

            // Log to console
            if (config_.console_output)
            {
                std::cout << formatted << std::endl;
            }

            // Log to file
            if (config_.file_output && log_file_ && log_file_->is_open())
            {
                *log_file_ << formatted << std::endl;
            }
        }

        bool Logger::should_log(LogLevel level) const
        {
            return static_cast<int>(level) >= static_cast<int>(current_level_);
        }

        std::string Logger::format_message(LogLevel level, const std::string &message, const std::string &function) const
        {
            std::ostringstream oss;

            oss << "[" << get_current_timestamp() << "]";
            oss << " [" << level_to_string(level) << "]";

            if (config_.log_function_name && !function.empty())
            {
                oss << " [" << function << "]";
            }

            oss << " " << message;

            return oss.str();
        }

        std::string Logger::level_to_string(LogLevel level) const
        {
            switch (level)
            {
            case LogLevel::LOG_TRACE:
                return "TRACE";
            case LogLevel::LOG_DEBUG:
                return "DEBUG";
            case LogLevel::LOG_INFO:
                return "INFO ";
            case LogLevel::LOG_WARN:
                return "WARN ";
            case LogLevel::LOG_ERROR:
                return "ERROR";
            case LogLevel::LOG_FATAL:
                return "FATAL";
            default:
                return "UNKN ";
            }
        }

        std::string Logger::get_current_timestamp() const
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch()) %
                      1000;

            std::ostringstream oss;
            oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            oss << "." << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        bool Logger::load_config(const std::string &config_file_path)
        {
            std::ifstream file(config_file_path);
            if (!file.is_open())
            {
                return false;
            }

            std::string line, current_section;
            while (std::getline(file, line))
            {
                // Skip empty lines and comments
                if (line.empty() || line[0] == ';' || line[0] == '#')
                    continue;

                // Handle sections
                if (line.front() == '[' && line.back() == ']')
                {
                    current_section = line.substr(1, line.length() - 2);
                    continue;
                }

                // Parse key=value
                size_t eq_pos = line.find('=');
                if (eq_pos == std::string::npos)
                    continue;

                std::string key = line.substr(0, eq_pos);
                std::string value = line.substr(eq_pos + 1);

                // Remove whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                // Apply configuration
                if (current_section == "Profiles")
                {
                    if (key == "Profile")
                    {
                        if (value == "std")
                            config_.profile = LogProfile::PROFILE_STD;
                        else if (value == "advanced")
                            config_.profile = LogProfile::PROFILE_ADVANCED;
                        else if (value == "verbose")
                            config_.profile = LogProfile::PROFILE_VERBOSE;
                    }
                }
                else if (current_section == "Logging")
                {
                    if (key == "LogDirectory")
                        config_.log_directory = value;
                    else if (key == "LogFileName")
                        config_.log_file_name = value;
                    else if (key == "MaxFileSize")
                        config_.max_file_size = value;
                    else if (key == "MaxBackupFiles")
                        config_.max_log_files = std::stoi(value);
                    else if (key == "EnableConsole")
                        config_.console_output = (value == "true" || value == "1");
                    else if (key == "EnableFile")
                        config_.file_output = (value == "true" || value == "1");
                }
            }

            return true;
        }

        void Logger::setup_log_directory()
        {
            if (!config_.log_directory.empty())
            {
                std::filesystem::create_directories(config_.log_directory);
            }
        }

        void Logger::open_log_file()
        {
            if (!config_.file_output)
                return;

            std::string full_path = get_full_log_path();
            log_file_ = std::make_unique<std::ofstream>(full_path, std::ios::app);
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