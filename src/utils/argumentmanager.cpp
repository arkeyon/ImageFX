#include "safpch.h"
#include "argumentmanager.h"

#include <fstream>

namespace saf {

    ArgumentManager::ArgumentManager(int argc, char* argsv[])
    {
        IFX_TRACE("ArgumentManager created");

        std::ifstream f("assets/params.json");
        if (f.is_open())
        {
            m_JSON = json::parse(f);
            IFX_TRACE("ArgumentManager loaded params.json\n{0}", m_JSON.dump(1, '\t'));
            ValidateJSON();
        }
        else IFX_WARN("Application was unable to open assets/params.json");
        
        if (!m_JSON.empty())
        {
            SetRunArguments(argc, argsv);
            ValidateArgs();

            if (m_RunArguments.empty()) IFX_TRACE("ArgumentManager was passed no run arguments");
            else
            {
                IFX_TRACE("ArgumentManager generator the following json containing run arguments\n{0}", m_RunArguments.dump(1, '\t'));
            }
        }
    }

    void ArgumentManager::ValidateJSON()
    {

    }

    //Reads run arguments and puts them in m_RunArguments.json
    void ArgumentManager::SetRunArguments(int argc, char* argsv[])
    {
        std::string current_parameter;
        std::vector<std::string> current_arguments;

        auto& unnamed = m_JSON["unnamed"];

        for (int i = 0; i < argc; ++i)
        {
            auto str = std::string(argsv[i]);
            if (str.starts_with('-'))
            {
                str = str.substr(1, str.size() - 1);
                if (current_parameter != "")
                {
                    if (current_arguments.size() == 0) m_RunArguments.emplace_back(current_parameter);
                    else if (current_arguments.size() == 1) m_RunArguments[current_parameter] = current_arguments[0];
                    else m_RunArguments[current_parameter] = current_arguments;
                }
                current_parameter = str;
                current_arguments.clear();
            }
            else
            {
                if (current_parameter == "")
                {
                    if (i < (int)unnamed.size()) m_RunArguments[unnamed[i]["name"]] = str;
                    else IFX_ERROR("ArgumentManager, too many unnamed arguments given, discarding \"{0}\"", str);
                }
                else current_arguments.emplace_back(str);

                if (i == argc - 1 && current_parameter != "")
                {
                    if (current_arguments.size() == 1) m_RunArguments[current_parameter] = current_arguments[0];
                    else m_RunArguments[current_parameter] = current_arguments;
                    current_arguments.clear();
                }
            }
        }
    }

    //Ensures that m_RunArguments only contains arguments defined in m_JSON (assets/params.json)
    void ArgumentManager::ValidateArgs()
    {
        if (m_RunArguments.empty() || m_JSON.empty()) return;

        auto& named = m_JSON["named"];
        auto& unnamed = m_JSON["unnamed"];

        std::vector<std::string> notfound;
        notfound.reserve(m_RunArguments.size());

        for (json::iterator it = m_RunArguments.begin(); it != m_RunArguments.end(); ++it)
        {
            bool found = false;
            for (auto& arg2 : unnamed)
            {
                if (it.key() == (std::string)arg2["name"])
                {
                    found = true;
                }
            }

            for (auto& arg2 : named)
            {
                if (it.key() == (std::string)arg2["name"])
                {
                    found = true;
                }
            }

            if (!found)
            {
                notfound.emplace_back(it.key());
            }
        }

        for (auto& str : notfound)
        {
            IFX_ERROR("argument {0} not recognised, removing it...", str);
            m_RunArguments.erase(str);
        }

    }

}