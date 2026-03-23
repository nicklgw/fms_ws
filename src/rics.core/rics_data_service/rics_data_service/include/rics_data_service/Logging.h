#ifndef ___RICS_LOGGING_HEADER___
#define ___RICS_LOGGING_HEADER___

#include <functional>
#include <string>
#include <cstdarg>
#include <cstdio>
#include <rclcpp/rclcpp.hpp>

// 根据是否定义 FULL_PATH 来决定如何获取文件名
#ifdef FULL_PATH
#define FILE_NAME(x) x
#else
#define FILE_NAME(x) (strrchr(x, '/') ? strrchr(x, '/') + 1 : x)
#endif

namespace rics_logging
{
    // 日志级别枚举
    enum LogSeverity {
        LOG_SEVERITY_TRACE = 5,
        LOG_SEVERITY_DEBUG = 4,
        LOG_SEVERITY_INFO = 3,
        LOG_SEVERITY_WARN = 2,
        LOG_SEVERITY_ERROR = 1,
        LOG_SEVERITY_FATAL = 0
    };

    // Logger 类，用于管理日志记录器名称
    class Logger {
    public:
        explicit Logger(const std::string &name = "rics_logging") : m_strName(name) {}
        const std::string &GetName() const { return m_strName; }
    private:
        std::string m_strName;
    };

    // Logging 类，提供静态方法进行日志记录
    class Logging {
    public:
        using LoggingCallback = std::function<void(LogSeverity severity, const char *file, const char *fun, int line, const char *pData)>;

        // 获取单例实例
        static Logging &Instance()
        {
            static Logging instance;
            return instance;
        }

        // 设置日志回调函数
        void OnLogging(LoggingCallback callback) { m_callback = std::move(callback); }

        // 各种级别的日志记录函数
        void Debug(const char *file, const char *fun, int line, const char *format, ...);
        void Info(const char *file, const char *fun, int line, const char *format, ...);
        void Warn(const char *file, const char *fun, int line, const char *format, ...);
        void Error(const char *file, const char *fun, int line, const char *format, ...);
        void Fatal(const char *file, const char *fun, int line, const char *format, ...);

    private:
        Logging() = default;

        // 处理日志消息的实际逻辑
        void HandleLogging(LogSeverity severity, const char *file, const char *fun, int line, const char *data);

        // 格式化日志消息
        void LogMessage(LogSeverity severity, const char *file, const char *fun, int line, const char *format, va_list args);

        LoggingCallback m_callback{nullptr};
    };

    // 获取全局日志记录器
    inline Logger &GetLogger()
    {
        static Logger logger;
        return logger;
    }

    // 宏定义，简化日志记录调用
    #define RICS_DEBUG(...) rics_logging::Logging::Instance().Debug(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
    #define RICS_INFO(...) rics_logging::Logging::Instance().Info(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
    #define RICS_WARN(...) rics_logging::Logging::Instance().Warn(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
    #define RICS_ERROR(...) rics_logging::Logging::Instance().Error(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
    #define RICS_FATAL(...) rics_logging::Logging::Instance().Fatal(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
}

#endif // ___RICS_LOGGING_HEADER___