#include "safpch.h"

#include "core/application.h"

class Example : public saf::Application
{
public:
    Example(const nlohmann::json& arguments)
        : Application(arguments)
    {

    }

    void Update() override
    {

    }
};

saf::Application* CreateApplication(nlohmann::json&& arguments) { return new Example(arguments); }