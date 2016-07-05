
#include <functional>
#include <vector>
#include <cstdarg>

#include "Log.h"

using namespace OpenT2T;


static std::function<void(LogSeverity severity, const char* message)> _logHandler;
static LogSeverity _severityLevel = LogSeverity::None;

void OpenT2T::SetLogHandler(std::function<void(LogSeverity severity, const char* message)> logHandler)
{
    _logHandler = logHandler;
}

void OpenT2T::SetLogLevel(LogSeverity severityLevel)
{
    _severityLevel = severityLevel;
}

void OpenT2T::details::LogMessage(LogSeverity severity, const char* message)
{
    if (severity <= _severityLevel && _logHandler)
    {
        _logHandler(severity, message);
    }
}

void OpenT2T::details::LogFormattedMessage(LogSeverity severity, const char* format, ...)
{
    va_list va_args;
    va_start(va_args, format);

    if (severity <= _severityLevel && _logHandler)
    {
        int length = vsnprintf(nullptr, 0, format, va_args);
        std::vector<char> buf(length + 1);
        vsnprintf(buf.data(), length + 1, format, va_args);
        _logHandler(severity, buf.data());
    }

    va_end(va_args);
}
