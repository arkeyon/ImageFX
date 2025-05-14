#pragma once

#include <memory>

#include "nlohmann/json.hpp"

#include "utils/argumentmanager.h"
#include "core/window.h"

#include "input/event.h"

#include "render/graphics.h"

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace saf {

    class Layer
    {
    public:
        inline virtual void Update() {}
        inline virtual void OnEvent(saf::Event& e) {}
        inline virtual void Render(std::shared_ptr<Renderer2D> renderer) {}
        inline virtual void ImGuiRender() {}
        inline virtual bool IsVisible() { return m_Visible; }
    protected:
        Layer() = default;

        bool m_Visible;
    };

    class DebugLayer : public Layer
    {
    public:
        virtual void Update() override;
        virtual void OnEvent(saf::Event& e) override;
        virtual void Render(std::shared_ptr<Renderer2D> renderer) override;
        virtual void ImGuiRender() override;
    private:
    };

    class Application
    {
    public:
        Application(const nlohmann::json& args, std::string title);
        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;
        virtual ~Application();

        virtual void Init() =0;
        virtual void Run();
        virtual void Update() = 0;
    protected:
        bool m_Running = true;
        nlohmann::json m_RunArgs;
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
        virtual void OnEvent(Event& e) =0;
        virtual void Update() = 0;

        inline void AddLayer(std::shared_ptr<Layer> layer) { m_Layers.push_back(layer); }

    protected:
        std::shared_ptr<Window> m_Window;
        std::shared_ptr<Input> m_Input;
        std::shared_ptr<Renderer2D> m_Renderer2D;
    private:
        std::vector<std::shared_ptr<Layer>> m_Layers;
        std::shared_ptr<DebugLayer> m_DebugLayer;

        glm::mat4 m_Projection2D;
    };

}