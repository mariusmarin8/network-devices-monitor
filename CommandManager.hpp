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
    bool authenticated = false;
    StorageManager* storage;
    json handleLogin(const json& task);
    json handleLogout();
    json handleGetStats(const json& task);
    json handleGetAgentAlerts(const json& task);
    json handleGetLogs(const json& task);
    json handleGetMetrics(const json& task);
    json handleFilterSyslog(const json& req);
    json handleFilterAgents(const json& req);
    json createResponse(string status, string message);
public:
    void setStorage(StorageManager* s);
    string process_command(const string& command);
};

