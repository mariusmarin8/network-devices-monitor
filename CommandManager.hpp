#pragma once

#include <iostream>
#include <nlohmann/json.hpp>
#include <string.h>

using namespace std;
using json = nlohmann::json;

class CommandManager
{
private:
    json handleLogin(const json& task);
    json handleGetStats(const json& task);
    json handleFilter(const json& task);
    json createResponse(std::string status, std::string message);
public:
    string process_command(const string& command);
};

