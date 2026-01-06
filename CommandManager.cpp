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
            if (response["status"] == "SUCCESS") {
                this->authenticated = true;
            }
            return response.dump();
        }else if (cmd == "LOGOUT") {
            response = handleLogout();
            return response.dump();
        }

        if (!this->authenticated) {
            return createResponse("ACCES NEAUTORIZAT", "Trebuie sa te autentifici mai intai").dump();
        }
        
        if (cmd == "GET_STATS") {
            response = handleGetStats(request);
        }
        else if (cmd == "FILTER_LOGS") {
            response = handleFilter(request);
        }else if (cmd == "GET_METRICS") {
            response = handleGetMetrics(request);
        }
        else if (cmd == "GET_LOGS"){
            response = handleGetLogs(request);
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
    if (!storage) 
        return createResponse("ERROR", "Storage neinitializat");
    
    string u = req.value("user", "");
    string p = req.value("pass", "");

    if(u.empty() || p.empty()){
        return createResponse("FAILED", "Username sau parola lipsa");
    }

    if (storage->auth(u, p)) {
        return createResponse("SUCCESS", "Login reusit");
    } else {
        return createResponse("FAILED", "Date invalide");
    }
}

json CommandManager::handleGetStats(const json& req) {
    if (!storage) 
        return createResponse("ERROR", "Storage neinitializat");

    json statsData = storage->getStats();

    json response = createResponse("CONFIRMED", "Statistici generate");
    response["data"] = statsData;
    
    return response;
    
}

json CommandManager::handleLogout() {
    if(this->authenticated == false){
        return createResponse("ERROR", "Nu este autentificat");
    }
    this->authenticated = false;
    return createResponse("SUCCESS", "Deconectare reusita");
}

json CommandManager::handleGetMetrics(const json& req) {
    if (!storage) 
        return createResponse("ERROR", "Storage neinitializat");

    int limit = req.value("limit", 50);
    string ip = req.value("ip", "ALL");
    json data = storage->getMetrics(ip, limit);
    
    json response = createResponse("CONFIRMED", "Toate metricile recuperate");
    response["data"] = data;
    
    return response;
}

json CommandManager::handleGetLogs(const json& req) {
    if (!storage) 
        return createResponse("ERROR", "Storage neinitializat");

    string ip = req.value("ip", "ALL");
    string sev = req.value("severity", "ALL");
    int limit = req.value("limit", 50);

    json data = storage->getLogs(ip, sev, limit);
    
    json response = createResponse("CONFIRMED", "Logs retrieved");
    response["data"] = data;
    return response;
}

json CommandManager::handleFilter(const json& req) {
    if (!storage) return createResponse("ERROR", "Storage neinitializat");

    string ip = req.value("ip", "ALL");
    string sev = req.value("severity", "ALL");
    int limit = req.value("limit", 20);

    json response = createResponse("CONFIRMED", "Date filtrate recuperate"); 
    response["data"]["logs"] = storage->getLogs(ip, sev, limit);
    response["data"]["metrics"] = storage->getMetrics(ip, limit);
    response["data"]["active_filters"] = { {"ip", ip}, {"severity", sev} };

    return response;
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