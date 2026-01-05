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
    return createResponse("CONFIRMED", "Cerere statistici primita.");
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