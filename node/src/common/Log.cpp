
#include <functional>
#include <vector>
#include <cstdarg>

#include "Log.h"

using namespace OpenT2T;

std::function<void(LogSeverity severity, const char* message)> OpenT2T::logHandler = nullptr;

LogSeverity OpenT2T::logLevel = LogSeverity::None;
