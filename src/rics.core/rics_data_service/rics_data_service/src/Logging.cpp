#include "rics_data_service/Logging.h"

#include <rclcpp/rclcpp.hpp>

namespace rics_logging {

void Logging::HandleLogging(LogSeverity severity, const char *file, const char *fun, int line,
                            const char *data) {
  if (m_callback) {
    m_callback(severity, file, fun, line, data);
  }
}

void Logging::LogMessage(LogSeverity severity, const char *file, const char *fun, int line,
                         const char *format, va_list args) {
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);

  // 使用 printf 风格的格式化字符串
  switch (severity) {
    case LOG_SEVERITY_DEBUG:
      RCLCPP_DEBUG(rclcpp::get_logger("data_collect_service"), "[%s:%d %s] %s", FILE_NAME(file),
                   line, fun, buffer);
      break;
    case LOG_SEVERITY_INFO:
      RCLCPP_INFO(rclcpp::get_logger("data_collect_service"), "[%s:%d %s] %s", FILE_NAME(file),
                  line, fun, buffer);
      break;
    case LOG_SEVERITY_WARN:
      RCLCPP_WARN(rclcpp::get_logger("data_collect_service"), "[%s:%d %s] %s", FILE_NAME(file),
                  line, fun, buffer);
      break;
    case LOG_SEVERITY_ERROR:
      RCLCPP_ERROR(rclcpp::get_logger("data_collect_service"), "[%s:%d %s] %s", FILE_NAME(file),
                   line, fun, buffer);
      break;
    case LOG_SEVERITY_FATAL:
      RCLCPP_FATAL(rclcpp::get_logger("data_collect_service"), "[%s:%d %s] %s", FILE_NAME(file),
                   line, fun, buffer);
      break;
    default:
      break;
  }

  HandleLogging(severity, file, fun, line, buffer);
}

void Logging::Debug(const char *file, const char *fun, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);
  LogMessage(LOG_SEVERITY_DEBUG, file, fun, line, format, args);
  va_end(args);
}

void Logging::Info(const char *file, const char *fun, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);
  LogMessage(LOG_SEVERITY_INFO, file, fun, line, format, args);
  va_end(args);
}

void Logging::Warn(const char *file, const char *fun, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);
  LogMessage(LOG_SEVERITY_WARN, file, fun, line, format, args);
  va_end(args);
}

void Logging::Error(const char *file, const char *fun, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);
  LogMessage(LOG_SEVERITY_ERROR, file, fun, line, format, args);
  va_end(args);
}

void Logging::Fatal(const char *file, const char *fun, int line, const char *format, ...) {
  va_list args;
  va_start(args, format);
  LogMessage(LOG_SEVERITY_FATAL, file, fun, line, format, args);
  va_end(args);
}

}  // namespace rics_logging