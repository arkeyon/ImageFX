#pragma once

#include <memory>

#include "nlohmann/json.hpp"

#include "utils/argumentmanager.h"
#include "core/window.h"

namespace saf {

    class Application
    {
    public:
        Application(const nlohmann::json& args);
        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;
        virtual ~Application() = default;

        void Init();
        virtual void Run();
        virtual void Update() =0;

        std::shared_ptr<Window> m_Window;
        std::shared_ptr<Renderer2D> m_Renderer2D;
    private:
        nlohmann::json m_RunArgs;
    };

}