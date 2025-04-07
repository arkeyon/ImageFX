#pragma once

#include <memory>

#include "nlohmann/json.hpp"

#include "utils/argumentmanager.h"
#include "core/window.h"

#include "input/event.h"

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace saf {

    class Application
    {
    public:
        Application(const nlohmann::json& args, std::string title);
        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;
        virtual ~Application();

        virtual void Init();
        virtual void Run();
        virtual void Update() = 0;
    private:
        nlohmann::json m_RunArgs;

    protected:
        bool m_Running = true;
    };

    class GraphicsApplication : public Application
    {
    public:
        GraphicsApplication(const nlohmann::json& args, std::string title);
        GraphicsApplication(const GraphicsApplication&) = delete;
        GraphicsApplication(GraphicsApplication&&) = delete;
        GraphicsApplication& operator=(const GraphicsApplication&) = delete;
        GraphicsApplication& operator=(GraphicsApplication&&) = delete;
        virtual ~GraphicsApplication();

        virtual void Init() override;
        virtual void Run() override;
        virtual void Update() = 0;

        std::shared_ptr<Window> m_Window;
        std::shared_ptr<Renderer2D> m_Renderer2D;
    };

}