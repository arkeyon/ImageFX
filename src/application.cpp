#include "application.h"
#include "logger.h"

Application::Application()
{

}

void Application::Run()
{
    if (m_RunArguments.empty()) IFX_TRACE("Application started with no run arguments");
    else
    {
        IFX_TRACE("Application started with " + std::to_string(m_RunArguments.size()) + " Arguments");
        for (const auto& param : m_RunArguments)
        {
            std::string param_args = param.first;
            for (const auto& arg : param.second)
            {
                if (arg == "") break;
                param_args += " " + arg;
            }
            IFX_TRACE(param_args);
        }
    }
}

//Add Entrypoint arguments to map m_RunArguments
void Application::SetRunArguments(int argc, char* argsv[])
{
    std::string current_parameter;
    std::array<std::string, 4> current_arguments;

    for (int i = 0, current_arguments_counter = 0; i < argc; ++current_arguments_counter, ++i)
    {
        auto str = std::string(argsv[i]);
        if (str.starts_with('-'))
        {
            if (i != 0 && current_parameter != "")
            {
                m_RunArguments[current_parameter] = current_arguments;
            }
            current_parameter = str;
            current_arguments_counter = -1;
        }
        else 
        {
            if (current_parameter != "") current_arguments[current_arguments_counter] = str;
            else if (i == 0) m_LaunchCommand = str;
            if (i == argc - 1) m_RunArguments[current_parameter] = current_arguments;
        }
    }
}

void Application::SetRunArguments(const std::string& parameter, std::string arg1, const std::string arg2, std::string arg3, std::string arg4)
{
    m_RunArguments[parameter] = std::array<std::string, 4>{arg1, arg2, arg3, arg4};
}

const std::array<std::string, 4>& Application::GetRunArgument(const std::string& param)
{
    return m_RunArguments[param];
}