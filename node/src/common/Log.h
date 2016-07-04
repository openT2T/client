// This is a lightweight cross-platform logging framework. The primary purpose is to route
// logging calls to appropriate platform-specific logs, e.g. Android logcat or iOS NSLog.

namespace OpenT2T
{

enum class LogSeverity
{
    None,
    Error,
    Warning,
    Info,
    Verbose,
    Trace,
};

// Implementation details, not intended to be called from elsewhere.
namespace details
{
    void LogMessage(LogSeverity severity, const char* message);
    void LogFormattedMessage(LogSeverity severity, const char* format, ...);
}

void SetLogHandler(std::function<void(LogSeverity severity, const char* message)> logHandler);

void SetLogLevel(LogSeverity severityLevel);

// Log a message, at the specified severity level.
inline void Log(LogSeverity severity, const char* message)
{
    details::LogMessage(severity, message);
}

// Log a message formatted with args, at the specified severity level.
template<class... Args>
inline void Log(LogSeverity severity, const char* format, Args&&... args)
{
    details::LogFormattedMessage(severity, format, std::forward<Args>(args)...);
}

// Define convenience methods for each severity level: LogError, LogWarning...
#define DECLARE_LOG_LEVEL(severity) \
    inline void Log##severity(const char* message) { details::LogMessage(LogSeverity::severity, message); } \
    template<class... Args> inline void Log##severity(const char* format, Args&&... args) { \
        details::LogFormattedMessage(LogSeverity::severity, format, std::forward<Args>(args)...); }

DECLARE_LOG_LEVEL(Error);
DECLARE_LOG_LEVEL(Warning);
DECLARE_LOG_LEVEL(Info);
DECLARE_LOG_LEVEL(Verbose);
DECLARE_LOG_LEVEL(Trace);

#undef DECLARE_LOG_LEVEL
}
