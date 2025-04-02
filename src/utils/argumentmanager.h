#pragma once

#include "nlohmann/json.hpp"

namespace saf {

    using json = nlohmann::json;

    class ArgumentManager
    {
    public:
        ArgumentManager(int argc, char* argsv[]);

        json m_RunArguments;
    private:
        void SetRunArguments(int argc, char* argsv[]); //enter entrypoint args into map
        void ValidateJSON(); //ensure params.json is valid
        void ValidateArgs();

        json m_JSON;
    };

}