#include "safpch.h"
#include "core/application.h"

extern saf::Application* CreateApplication(nlohmann::json&& arguments);

int main(int argc, char** argsv)
{
    saf::log::Init();

    saf::Application* app = nullptr;
    {
        saf::ArgumentManager argsman(argc, argsv);
        app = CreateApplication(std::move(argsman.m_RunArguments));
    }

    app->Init();
    app->Run();

    IFX_INFO("Application exiting...");

    delete app;

    return 0;
}