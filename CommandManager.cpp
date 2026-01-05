#include "CommandManager.hpp"

string CommandManager::process_command(const string& command){
    json response;
    
    try {
        json request = json::parse(command);
        
        if (!request.contains("command")) {
            return createResponse("ERROR", "Lipseste campul 'command'").dump();
        }

        std::string cmd = request["command"];

        if (cmd == "LOGIN") {
            response = handleLogin(request);
        }
        else if (cmd == "GET_STATS") {
            response = handleGetStats(request);
        }
        else if (cmd == "FILTER_LOGS") {
            response = handleFilter(request);
        }
        else {
            response = createResponse("UNKNOWN", "Comanda nerecunoscuta: " + cmd);
        }
        return response.dump();

    }catch (json::parse_error& e) {
        return createResponse("ERROR", "JSON Invalid").dump();
    }
}

json CommandManager::handleLogin(const json& req) {
    std::string user = req.value("user", "unknown");
    
    return createResponse("CONFIRMED", "Login procesat pentru user: " + user);
}

json CommandManager::handleGetStats(const json& req) {
    if (!storage) 
        return createResponse("ERROR", "Storage neinitializat");

    auto logs = storage -> getLogs();

    int err = 0;
    int warn = 0;

    for(const auto& log : logs) {
        if(log.getSeverityNr() <= 3) 
            err++;
        else if(log.getSeverityNr() == 4) 
            warn++;
    }

    json stats;
    stats["total_logs"] = logs.size();
    stats["errors"] = err;
    stats["warnings"] = warn;
    
    json response = createResponse("CONFIRMED", "Statistici generate");
    response["data"] = stats;
    return response;
    
}

json CommandManager::handleFilter(const json& req) {
    std::string ip = req.value("ip", "ALL");
    std::string sev = req.value("severity", "ALL");
    
    return createResponse("CONFIRMED", "Filtru aplicat pe IP: " + ip + " cu severity: " + sev);
}

json CommandManager::createResponse(std::string status, std::string message) {
    json j;
    j["status"] = status;
    j["message"] = message;
    return j;
}

void CommandManager::setStorage(StorageManager* s) { 
    this->storage = s; 
}