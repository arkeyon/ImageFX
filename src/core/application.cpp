#include "safpch.h"
#include <memory>
#include "application.h"

namespace saf {

    Application::Application(const nlohmann::json& args)
        : m_RunArgs(args), m_Window(std::make_unique<Window>(1024, 768, "Example"))
    {
        IFX_TRACE("Application created");
    }

    void Application::Init()
    {
        IFX_INFO("Application Init");
    }

    void Application::Run()
    {
        IFX_INFO("Application Run");

        while(!m_Window->ShouldClose())
        {
            Update();
            m_Window->Update();
        }

    }

}