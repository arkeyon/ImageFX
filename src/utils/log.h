#pragma once

#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>
#include <thread>
#include <memory>
#include <type_traits>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/ansicolor_sink.h"

#define IFX_TRACE(...)    saf::log::g_Logger->trace(__VA_ARGS__)
#define IFX_WARN(...)    saf::log::g_Logger->warn(__VA_ARGS__)
#define IFX_ERROR(...)    saf::ifx_error(__VA_ARGS__)
#define IFX_INFO(...)    saf::log::g_Logger->info(__VA_ARGS__)

//#define IFX_TRACE(x)    (std::cout << "[" << TraceID() << "]   " << appname << "_trace: " << x << std::endl)
//#define IFX_WARNING(x)  (std::cout << "[" << TraceID() << "] " << appname << "_warning: " << x << std::endl)
//#define IFX_ERROR(x)    (std::cout << "[" << TraceID() << "]   " << appname << "_error: " << x << std::endl)
//#define IFX_INFO(x)     (std::cout << "[" << TraceID() << "]    " << appname << "_info: " << x << std::endl)

namespace saf {


    namespace log {

        extern std::shared_ptr<spdlog::logger> g_CoreLogger;
        extern std::shared_ptr<spdlog::logger> g_Logger;

        void Init();

    }

    template <typename T>
    void ifx_error(const T& msg) {
        saf::log::g_Logger->log(spdlog::level::err, msg);
        exit(1);
    }

    template <typename... Args>
    void ifx_error(spdlog::format_string_t<Args...> fmt, Args &&...args)
    {
        saf::log::g_Logger->log(spdlog::level::err, fmt, std::forward<Args>(args)...);
        exit(1);
    }

    [[nodiscard]] inline const int64_t& TraceID()
    {
        auto clock = std::chrono::system_clock::now();
        auto duration = clock.time_since_epoch();
        int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        static int64_t last = now;
        if (now <= last) return ++last;
        return last = now;
    }

    const std::string appname = "imagefx";

}