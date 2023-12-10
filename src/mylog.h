#pragma once
#include "raylog.h"

namespace ray
{

    inline ray::log::Logger logError(const std::source_location& location = std::source_location::current())
    {
        return ray::log::Logger(location.file_name(), location.line(), ray::log::Level::Error);
    }

    inline ray::log::Logger logInfo(const std::source_location& location = std::source_location::current())
    {
#ifdef QT_DEBUG
        return ray::log::Logger(location.file_name(), location.line(), ray::log::Level::Debug);
#else
        return ray::log::Logger(location.file_name(), location.line(), ray::log::Level::Info);
#endif
    }
}