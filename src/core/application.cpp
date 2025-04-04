#include "safpch.h"
#include <memory>
#include "application.h"

#include "globals.h"

namespace saf {

    Application::Application(const nlohmann::json& args)
        : m_RunArgs(args)
    {
        m_Window = std::make_shared<Window>(1024, 768, "Example");

        m_Renderer2D = std::make_shared<Renderer2D>(1024, 768);
        global::g_Renderer2D = m_Renderer2D;
    }

    Application::~Application()
    {
        m_Renderer2D->Shutdown();
    }

    void Application::Init()
    {
        IFX_INFO("Application Init");

        m_Window->Init();
        m_Renderer2D->Init();
    }

    const int maxfps = 60;

    void Application::Run()
    {
        IFX_INFO("Application Run");

        while(!m_Window->ShouldClose())
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

}