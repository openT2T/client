// This is a lightweight cross-platform logging framework. The primary purpose is to route
// logging calls to appropriate platform-specific logs, e.g. Android logcat or iOS NSLog.

namespace OpenT2T
{

// Specifies the severity of a log message.
enum class LogSeverity
{
    None,
    Error,
    Warning,
    Info,
    Verbose,
    Trace,
};

// Implementation methods in the "details" namespace should be treated as private.
namespace details
{
    void LogMessage(LogSeverity severity, const char* message);
    void LogFormattedMessage(LogSeverity severity, const char* format, ...);
}

// Registers a function to be invoked with the log severity and message whenever OpenT2T::Log() is invoked.
// Typically this is used to route logging calls to appropriate platform-specific logs, e.g. Android logcat
// or iOS NSLog. By default, no handler is registered, meaning logging calls are simply ignored.
void SetLogHandler(std::function<void(LogSeverity severity, const char* message)> logHandler);

// Sets the required severity level for logging calls to be passed on to the registered log handler, if any.
// The default is LogSeverity::None, meaning no calls are passed on. For example, setting the log level to
// Warning causes Error and Warning calls to be passed on, while Verbose and Trace calls are ignored.
void SetLogLevel(LogSeverity severityLevel);

// Logs a message at the specified severity level.
inline void Log(LogSeverity severity, const char* message)
{
    details::LogMessage(severity, message);
}

// Logs a message formatted with args at the specified severity level.
// (Implementation notes: This uses C++ arg forwarding rather than C VA_ARGS to allow for overload detection
// to distinguish calls that require formatting from (cheaper) calls that don't. But the implementation function
// in the details namespace doesn't, to avoid issues with template instantiation.)
template<class... Args>
inline void Log(LogSeverity severity, const char* format, Args&&... args)
{
    details::LogFormattedMessage(severity, format, std::forward<Args>(args)...);
}

// Define convenience methods for each severity level, named LogError, LogWarning...
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
