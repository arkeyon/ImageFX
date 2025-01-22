#pragma once

#include <memory>
#include <unordered_map>

#include "json/single_include/nlohmann/json.hpp"

#include "utils/argumentmanager.h"
#include "core/window.h"

namespace saf {

    class Application
    {
    public:
        Application(const nlohmann::json& args);
        void Init();
        virtual void Run();
        virtual void Update() =0;

        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;
        virtual ~Application() = default;
    private:
        nlohmann::json m_RunArgs;
        std::unique_ptr<Window> m_Window;
    };

}