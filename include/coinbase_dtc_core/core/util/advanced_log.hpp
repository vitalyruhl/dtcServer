#pragma once
#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <chrono>
#include <filesystem>

namespace open_dtc_server
{
    namespace util
    {

        // Standard Log Levels (compatible with log4j/spdlog/syslog standards)
        enum class LogLevel
        {
            TRACE = 0, // Most verbose - everything (equivalent to TRACE)
            DEBUG = 1, // Debug information (equivalent to DEBUG)
            INFO = 2,  // General information (equivalent to INFO)
            WARN = 3,  // Warning messages (equivalent to WARN)
            ERROR = 4, // Error messages (equivalent to ERROR)
            FATAL = 5  // Critical/Fatal errors (equivalent to FATAL)
        };

        // Log Level Profiles (mapped to standard levels)
        enum class LogProfile
        {
            VERBOSE = 0,  // verbose: Everything (TRACE and above)
            ADVANCED = 1, // advanced: Info and above (INFO, WARN, ERROR, FATAL)
            STD = 2       // std: Only errors and fatal (ERROR, FATAL)
        };

        // Logging Configuration
        struct LogConfig
        {
            LogProfile profile = LogProfile::ADVANCED;
            std::string log_directory = "logs";
            std::string log_file_name = "dtc_server.log";
            std::string max_file_size = "500MB"; // Size before rotation
            int max_log_files = 10;
            bool rotate_on_startup = true;
            bool rotate_on_size = true;
            bool compress_old_logs = true;
            bool console_output = true;
            bool file_output = true;
            std::string timestamp_format = "%Y-%m-%d %H:%M:%S.%f";
            std::string log_format = "[{timestamp}] [{level:>5}] {message}";

            // Advanced settings
            bool log_thread_id = true;
            bool log_function_name = false;
            bool log_file_location = true;
            bool log_line_number = false;
            size_t buffer_size = 4096;
            int flush_interval_ms = 1000;

            // Performance settings
            bool log_performance = false;
            bool log_memory_usage = false;
            bool log_network_activity = true;
            bool log_slow_operations = true;
            int slow_operation_threshold_ms = 100;

            // Debug settings (active only in TRACE/DEBUG levels)
            bool log_dtc_messages = true;
            bool log_coinbase_api = true;
            bool log_websocket_data = false;
            bool log_market_data = true;
            bool log_request_response = true;
            bool log_stack_trace = false;
        };

        class Logger
        {
        public:
            static Logger &getInstance();

            // Configuration
            bool initialize(const std::string &config_file_path = "config/logging.ini");
            void setLogProfile(LogProfile profile);
            void setLogLevel(LogLevel level);
            LogLevel getLogLevel() const { return current_level_; }
            LogProfile getLogProfile() const { return config_.profile; }

            // Core logging functions (standard level names)
            void trace(const std::string &message, const std::string &function = "");
            void debug(const std::string &message, const std::string &function = "");
            void info(const std::string &message, const std::string &function = "");
            void warn(const std::string &message, const std::string &function = ""); // Standard: warn
            void error(const std::string &message, const std::string &function = "");
            void fatal(const std::string &message, const std::string &function = ""); // Standard: fatal

            // Legacy compatibility functions
            void warning(const std::string &message, const std::string &function = "") { warn(message, function); }
            void critical(const std::string &message, const std::string &function = "") { fatal(message, function); }

            // Specialized logging functions
            void log_dtc_message(const std::string &direction, const std::string &message_type, const std::string &content);
            void log_coinbase_api(const std::string &endpoint, const std::string &method, const std::string &response);
            void log_websocket_data(const std::string &direction, const std::string &data);
            void log_market_data(const std::string &symbol, const std::string &data);
            void log_performance(const std::string &operation, double duration_ms);
            void log_network_activity(const std::string &activity, const std::string &details);

            // Log rotation and maintenance
            void rotate_logs();
            void rotate_if_size_exceeded(); // NEW: Check and rotate based on file size
            void cleanup_old_logs();

            // Utility functions
            size_t parse_size_string(const std::string &size_str) const; // NEW: Parse "500MB" etc
            size_t get_current_log_file_size() const;                    // NEW: Get current log file size
            std::string get_full_log_path() const;                       // NEW: Get complete log file path
            void flush();
            void shutdown();

        private:
            Logger() = default;
            ~Logger();

            // Prevent copying
            Logger(const Logger &) = delete;
            Logger &operator=(const Logger &) = delete;

            // Internal logging
            void log_internal(LogLevel level, const std::string &message, const std::string &function = "");
            bool should_log(LogLevel level) const;
            std::string format_message(LogLevel level, const std::string &message, const std::string &function) const;
            std::string level_to_string(LogLevel level) const;
            std::string get_current_timestamp() const;

            // Configuration and file management
            bool load_config(const std::string &config_file_path);
            void setup_log_directory();
            void open_log_file();
            void close_log_file();

            // Member variables
            LogConfig config_;
            LogLevel current_level_ = LogLevel::INFO;
            std::unique_ptr<std::ofstream> log_file_;
            std::mutex log_mutex_;
            bool initialized_ = false;
        };

        // Convenience macros for easy logging with automatic function names (standard levels)
#define LOG_TRACE(msg) open_dtc_server::util::Logger::getInstance().trace(msg, __FUNCTION__)
#define LOG_DEBUG(msg) open_dtc_server::util::Logger::getInstance().debug(msg, __FUNCTION__)
#define LOG_INFO(msg) open_dtc_server::util::Logger::getInstance().info(msg, __FUNCTION__)
#define LOG_WARN(msg) open_dtc_server::util::Logger::getInstance().warn(msg, __FUNCTION__)
#define LOG_ERROR(msg) open_dtc_server::util::Logger::getInstance().error(msg, __FUNCTION__)
#define LOG_FATAL(msg) open_dtc_server::util::Logger::getInstance().fatal(msg, __FUNCTION__)

// Legacy compatibility macros
#define LOG_WARNING(msg) LOG_WARN(msg)
#define LOG_CRITICAL(msg) LOG_FATAL(msg) // Specialized logging macros
#define LOG_DTC_MESSAGE(dir, type, content)                                                                         \
    if (open_dtc_server::util::Logger::getInstance().getLogProfile() >= open_dtc_server::util::LogProfile::VERBOSE) \
    open_dtc_server::util::Logger::getInstance().log_dtc_message(dir, type, content)

#define LOG_COINBASE_API(endpoint, method, response)                                                                 \
    if (open_dtc_server::util::Logger::getInstance().getLogProfile() >= open_dtc_server::util::LogProfile::ADVANCED) \
    open_dtc_server::util::Logger::getInstance().log_coinbase_api(endpoint, method, response)

#define LOG_MARKET_DATA(symbol, data)                                                                               \
    if (open_dtc_server::util::Logger::getInstance().getLogProfile() >= open_dtc_server::util::LogProfile::VERBOSE) \
    open_dtc_server::util::Logger::getInstance().log_market_data(symbol, data)

#define LOG_PERFORMANCE(operation, duration) \
    open_dtc_server::util::Logger::getInstance().log_performance(operation, duration)

        // Legacy compatibility functions
        inline void simple_log(const std::string &message)
        {
            Logger::getInstance().info(message);
        }

        inline void write_log(const std::string &message)
        {
            Logger::getInstance().info(message);
        }

        inline void log_debug(const std::string &message)
        {
            Logger::getInstance().debug(message);
        }

        inline void log_info(const std::string &message)
        {
            Logger::getInstance().info(message);
        }

        inline void log_warning(const std::string &message)
        {
            Logger::getInstance().warning(message);
        }

        inline void log_error(const std::string &message)
        {
            Logger::getInstance().error(message);
        }

    } // namespace util
} // namespace open_dtc_server