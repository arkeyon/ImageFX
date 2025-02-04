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
    
    try {

        app->Init();
        app->Run();

    }
    catch (const std::exception& e)
    {
        IFX_TRACE(e.what());
    }

    IFX_INFO("Application exiting...");

    delete app;

    return 0;
}