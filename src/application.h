#include <array>
#include <unordered_map>
#include <vector>
#include <string>

class Application
{
public:
    Application();
    void Run();

    void SetRunArguments(int argc, char* argsv[]);
    void SetRunArguments(const std::string& parameter, std::string arg1, const std::string arg2 = "", std::string arg3 = "", std::string arg4 = "");

    const std::array<std::string, 4>& GetRunArgument(const std::string& param);

private:
    std::unordered_map<std::string, std::array<std::string, 4>> m_RunArguments;
    std::string m_LaunchCommand;
};