#include "safpch.h"
#include "log.h"

namespace saf { 
    namespace log {

        std::shared_ptr<spdlog::logger> g_CoreLogger;
        std::shared_ptr<spdlog::logger> g_Logger;

        void Init()
        {

            g_CoreLogger = spdlog::stdout_color_mt("saf");
            g_CoreLogger->set_level(spdlog::level::trace);

            spdlog::sink_ptr sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
            g_Logger = std::make_shared<spdlog::logger>("app", sink);
            g_Logger->set_level(spdlog::level::trace);
        }

    }
}