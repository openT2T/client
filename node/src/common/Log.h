
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

void SetLogHandler(std::function<void(LogSeverity severity, const char* message)> logHandler);

void SetLogLevel(LogSeverity severityLevel);

void Log(LogSeverity severity, const char* message);

template<class... Args>
void Log(LogSeverity severity, const char* format, Args&&... args);

// Define convenience methods for each severity level: LogError, LogWarning...
#define DECLARE_LOG_LEVEL(severity) \
    inline void Log##severity(const char* message) { Log(LogSeverity::severity, message); } \
    template<class... Args> inline void Log##severity(const char* format, Args&&... args) { \
        Log(LogSeverity::severity, format, std::forward<Args>(args)...); }

DECLARE_LOG_LEVEL(Error);
DECLARE_LOG_LEVEL(Warning);
DECLARE_LOG_LEVEL(Info);
DECLARE_LOG_LEVEL(Verbose);
DECLARE_LOG_LEVEL(Trace);

#undef DECLARE_LOG_LEVEL
}
