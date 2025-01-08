#include <iostream>

#include "logger.h"
#include "application.h"

int main(int argc, char* argsv[])
{
    Application* app = new Application();
    app->SetRunArguments(argc, argsv);
    app->Run();

    return 0;
}