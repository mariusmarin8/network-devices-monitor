#pragma once

#include <iostream>
#include <nlohmann/json.hpp>
#include <string.h>
#include "StorageManager.hpp"
using namespace std;
using json = nlohmann::json;

class CommandManager
{
private:
    StorageManager* storage;
    json handleLogin(const json& task);
    json handleGetStats(const json& task);
    json handleGetLogs(const json& task);
    json handleGetMetrics(const json& task);
    json handleFilter(const json& task);
    json createResponse(std::string status, std::string message);
public:
    void setStorage(StorageManager* s);
    string process_command(const string& command);
};

