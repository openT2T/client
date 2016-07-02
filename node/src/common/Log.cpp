
#include <functional>
#include <vector>

#include "Log.h"

using namespace OpenT2T;

static std::function<void(LogSeverity severity, const char* message)> _logHandler;
static LogSeverity _severityLevel = LogSeverity::None;

void SetLogHandler(std::function<void(LogSeverity severity, const char* message)> logHandler)
{
    _logHandler = logHandler;
}

void SetLogLevel(LogSeverity severityLevel)
{
    _severityLevel = severityLevel;
}

void Log(LogSeverity severity, const char* message)
{
    if (severity <= _severityLevel && _logHandler)
    {
        _logHandler(severity, message);
    }
}

template<class... Args>
void Log(LogSeverity severity, const char* format, Args&&... args)
{
    if (severity <= _severityLevel && _logHandler)
    {
        int length = vsnprintf(nullptr, 0, format, std::forward<Args>(args)...);
        std::vector<char> buf(length + 1);
        vsnprintf(buf.data(), length + 1, std::forward<Args>(args)...);
        _logHandler(severity, buf.data());
    }
}
