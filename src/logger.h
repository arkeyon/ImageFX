
#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>
#include <thread>

inline const int64_t& TraceID()
{
    auto clock = std::chrono::system_clock::now();
    auto duration = clock.time_since_epoch();
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    static int64_t last = now;
    if (now <= last) return ++last;
    return last = now;
}

#define IFX_TRACE(x) (std::cout << "[" << TraceID() << "] imagefx_trace: " << x << std::endl)