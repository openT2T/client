// This is a lightweight cross-platform logging framework. The primary purpose is to route
// logging calls to appropriate platform-specific logs, e.g. Android logcat or iOS NSLog.

namespace OpenT2T
{

/// Specifies the severity of a log message.
enum class LogSeverity
{
    None,
    Error,
    Warning,
    Info,
    Verbose,
    Trace,
};

/// Function to be invoked with the log severity and message whenever OpenT2T::Log() is invoked.
/// Typically this is used to route logging calls to appropriate platform-specific logs, e.g. Android
/// logcat or iOS NSLog. By default the handler is null, meaning logging calls are simply ignored.
extern std::function<void(LogSeverity severity, const char* message)> logHandler;

/// Severity level for logging calls to be passed on to the registered log handler, if any. The default
/// is LogSeverity::None, meaning no calls are passed on. For example, setting this log level to Warning
/// causes Error and Warning calls to be passed on, while Info, Verbose, and Trace calls are suppressed.
extern LogSeverity logLevel;

/// Logs a message at the specified severity level.
inline void Log(LogSeverity severity, const char* message)
{
    if (severity <= logLevel && logHandler != nullptr)
    {
        logHandler(severity, message);
    }
}

/// Logs a message formatted with args at the specified severity level.
template <typename... Args>
inline void Log(LogSeverity severity, const char* format, Args&&... args)
{
    if (severity <= logLevel && logHandler != nullptr)
    {
        int length = snprintf(nullptr, 0, format, std::forward<Args>(args)...);
        std::vector<char> buf(length + 1);
        snprintf(buf.data(), length + 1, format, std::forward<Args>(args)...);
        logHandler(severity, buf.data());
    }
}

// Define convenience methods for each severity level, named LogError, LogWarning...
#define DECLARE_LOG_LEVEL(severity) \
    inline void Log##severity(const char* message) { Log(LogSeverity::severity, message); } \
    template <typename... Args> inline void Log##severity(const char* format, Args&&... args) { \
        Log(LogSeverity::severity, format, std::forward<Args>(args)...); }

DECLARE_LOG_LEVEL(Error);
DECLARE_LOG_LEVEL(Warning);
DECLARE_LOG_LEVEL(Info);
DECLARE_LOG_LEVEL(Verbose);
DECLARE_LOG_LEVEL(Trace);

#undef DECLARE_LOG_LEVEL
}
