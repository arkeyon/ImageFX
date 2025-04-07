#include "safpch.h"
#include <memory>
#include "application.h"

#include "globals.h"

namespace saf {

    Application::Application(const nlohmann::json& args, std::string title)
        : m_RunArgs(args)
    {

    }

    Application::~Application()
    {

    }

    GraphicsApplication::GraphicsApplication(const nlohmann::json& args, std::string title)
        : Application(args, title)
    {
        m_Window = std::make_shared<Window>(1024, 768, title);

        m_Renderer2D = std::make_shared<Renderer2D>(1024, 768);
        global::g_Renderer2D = m_Renderer2D;
    }

    GraphicsApplication::~GraphicsApplication()
    {
        m_Renderer2D->Shutdown();
    }

    void GraphicsApplication::Init()
    {
        IFX_INFO("Application Init");

        m_Window->Init();
        m_Renderer2D->Init();
    }

    void Application::Init()
    {

    }

    const int maxfps = 60;

    void GraphicsApplication::Run()
    {
        IFX_INFO("Application Run");

        while(!m_Window->ShouldClose() && m_Running)
        {
            static auto oldTime = std::chrono::high_resolution_clock::now();
            const int us = 1000000 / maxfps;

            if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - oldTime) < std::chrono::microseconds{ us })
            {
                continue;
            }
            oldTime = std::chrono::high_resolution_clock::now();

            Update();
            m_Window->Update();
        }

    }

    void Application::Run()
    {
        IFX_INFO("Application Run");

        while (m_Running)
        {
            Update();
        }

    }

}