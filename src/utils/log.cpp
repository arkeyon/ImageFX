#include "safpch.h"
#include "log.h"

namespace saf { 
    namespace log {

        std::shared_ptr<spdlog::logger> g_CoreLogger;
        std::shared_ptr<spdlog::logger> g_Logger;

        void Init()
        {
            spdlog::set_pattern("[%T]%n-%L: %v");
            
            g_CoreLogger = spdlog::stdout_color_mt("saf");
            g_CoreLogger->set_level(spdlog::level::trace);

            g_Logger = spdlog::stdout_color_mt("app");
            g_Logger->set_level(spdlog::level::trace);
        }

    }
}