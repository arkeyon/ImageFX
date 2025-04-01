/*
#include "safpch.h"

#include "core/application.h"

#include "globals.h"

class Example : public saf::Application
{
public:
    Example(const nlohmann::json& arguments)
        : Application(arguments)
    {

    }

    void Update() override
    {
        saf::global::g_Renderer2D->BeginScene();

        saf::global::g_Renderer2D->DrawString(m_Window->str, 1.f / static_cast<float>(m_Window->GetHeight()), glm::vec2(-0.9f, -0.9f), glm::vec2(0.9f, 0.9f));

        saf::global::g_Renderer2D->EndScene();
    }
};

saf::Application* CreateApplication(nlohmann::json&& arguments) { return new Example(arguments); }
*/