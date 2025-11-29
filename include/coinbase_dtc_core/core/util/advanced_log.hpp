#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <filesystem>

namespace open_dtc_server
{
    namespace util
    {
        enum class LogLevel : uint16_t
        {
            LOG_TRACE = 0,
            LOG_DEBUG = 1,
            LOG_INFO = 2,
            LOG_WARN = 3,
            LOG_ERROR = 4,
            LOG_FATAL = 5
        };

        enum class LogProfile : uint16_t
        {
            PROFILE_VERBOSE = 0,
            PROFILE_ADVANCED = 1,
            PROFILE_STD = 2
        };

        struct LogConfig
        {
            LogProfile profile = LogProfile::PROFILE_ADVANCED;
            std::string log_directory = "logs";
            std::string log_file_name = "dtc_server.log";
            std::string max_file_size = "500MB";
            int max_log_files = 10;
            bool rotate_on_startup = true;
            bool rotate_on_size = true;
            bool console_output = true;
            bool file_output = true;
            bool log_thread_id = true;
            bool log_function_name = false;
            bool log_dtc_messages = true;
            bool log_coinbase_api = true;
            bool log_websocket_data = false;
            bool log_market_data = true;
            bool log_request_response = true;
        };

        class Logger
        {
        public:
            static Logger &getInstance();

            bool initialize(const std::string &config_file_path = "config/logging.ini");
            void setLogProfile(LogProfile profile);
            void setLogLevel(LogLevel level);
            LogLevel getLogLevel() const { return current_level_; }
            LogProfile getLogProfile() const { return config_.profile; }

            void trace(const std::string &message, const std::string &function = "");
            void debug(const std::string &message, const std::string &function = "");
            void info(const std::string &message, const std::string &function = "");
            void warn(const std::string &message, const std::string &function = "");
            void error(const std::string &message, const std::string &function = "");
            void fatal(const std::string &message, const std::string &function = "");

            void warning(const std::string &message, const std::string &function = "") { warn(message, function); }
            void critical(const std::string &message, const std::string &function = "") { fatal(message, function); }

            void log_dtc_message(const std::string &direction, const std::string &message_type, const std::string &content);
            void log_coinbase_api(const std::string &endpoint, const std::string &method, const std::string &response);
            void log_websocket_data(const std::string &direction, const std::string &data);
            void log_market_data(const std::string &symbol, const std::string &data);
            void log_performance(const std::string &operation, double duration_ms);
            void log_network_activity(const std::string &activity, const std::string &details);

            void rotate_logs();
            void rotate_logs_unsafe();
            void rotate_if_size_exceeded();
            void cleanup_old_logs();

            size_t parse_size_string(const std::string &size_str) const;
            size_t get_current_log_file_size() const;
            std::string get_full_log_path() const;
            void flush();
            void shutdown();

        private:
            Logger() = default;
            ~Logger();

            Logger(const Logger &) = delete;
            Logger &operator=(const Logger &) = delete;

            void log_internal(LogLevel level, const std::string &message, const std::string &function = "");
            void log_internal_unsafe(LogLevel level, const std::string &message, const std::string &function = "");
            bool should_log(LogLevel level) const;
            std::string format_message(LogLevel level, const std::string &message, const std::string &function) const;
            std::string level_to_string(LogLevel level) const;
            std::string get_current_timestamp() const;

            bool load_config(const std::string &config_file_path);
            void setup_log_directory();
            void open_log_file();
            void close_log_file();

            LogConfig config_;
            LogLevel current_level_ = LogLevel::LOG_INFO;
            std::unique_ptr<std::ofstream> log_file_;
            std::mutex log_mutex_;
            bool initialized_ = false;
        };

#define LOG_TRACE(msg) open_dtc_server::util::Logger::getInstance().trace(msg, __FUNCTION__)
#define LOG_DEBUG(msg) open_dtc_server::util::Logger::getInstance().debug(msg, __FUNCTION__)
#define LOG_INFO(msg) open_dtc_server::util::Logger::getInstance().info(msg, __FUNCTION__)
#define LOG_WARN(msg) open_dtc_server::util::Logger::getInstance().warn(msg, __FUNCTION__)
#define LOG_ERROR(msg) open_dtc_server::util::Logger::getInstance().error(msg, __FUNCTION__)
#define LOG_FATAL(msg) open_dtc_server::util::Logger::getInstance().fatal(msg, __FUNCTION__)

    } // namespace util
} // namespace open_dtc_server