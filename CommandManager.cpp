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
        } else if (cmd == "LOGOUT") {
            response = handleLogout();
            return response.dump();
        }

        if (!this->authenticated) {
            return createResponse("ACCES NEAUTORIZAT", "Trebuie sa te autentifici mai intai").dump();
        }
        
        if (cmd == "FILTER_AGENTS") {
            response = handleFilterAgents(request);
        }
     
        else if (cmd == "FILTER_LOGS") {
            response = handleFilterSyslog(request);
        }

        else if (cmd == "GET_STATS") {
            response = handleGetStats(request);
        }
        else if (cmd == "GET_METRICS") {
            response = handleGetMetrics(request);
        }
        else if (cmd == "GET_LOGS"){
            response = handleGetLogs(request);
        }else if (cmd == "GET_AGENT_LIST") {
            json list = storage->getAgentList();
            json response = createResponse("CONFIRMED", "Lista agenti recuperata");
            response["data"] = list;
            return response.dump();
        }
        else if (cmd == "GET_SYSLOG_IPS") {
            json list = storage->getSyslogIPs();
            json response = createResponse("CONFIRMED", "Lista IP-uri Syslog recuperata");
            response["data"] = list; 
            return response.dump();
        }else if (cmd == "GET_AGENT_ALERTS") {
            response = handleGetAgentAlerts(request);
        }
        else {
            response = createResponse("UNKNOWN", "Comanda nerecunoscuta: " + cmd);
        }
        return response.dump();

    } catch (json::parse_error& e) {
        return createResponse("ERROR", "JSON Invalid").dump();
    }
}


json CommandManager::handleFilterAgents(const json& req) {
    if (!storage) return createResponse("ERROR", "Storage neinitializat");

    string agent = req.value("agent", "ALL"); 
    string sev = req.value("severity", "ALL");
    string msg = req.value("search", "");
    int limit = req.value("limit", 50);

    json response = createResponse("CONFIRMED", "Date Agent recuperate"); 
    json data;
    data["alerts"] = storage->getAgentAlerts(agent, sev, limit); 

    data["metrics"] = storage->getMetrics(agent, limit);

    data["active_filters"] = { 
        {"agent", agent}, 
        {"severity", sev}, 
        {"search", msg} 
    };

    response["data"] = data;
    return response;
}

json CommandManager::handleFilterSyslog(const json& req) {
    if (!storage) return createResponse("ERROR", "Storage neinitializat");

    string ip = req.value("ip", "ALL");
    string sev = req.value("severity", "ALL");
    string msg = req.value("search", "");
    int limit = req.value("limit", 50);

    json response = createResponse("CONFIRMED", "Syslogs recuperate");
    json data;
    data["logs"] = storage->getLogs(ip, sev, msg, limit);
    data["stats"] = storage->getStats(ip);

    data["active_filters"] = { 
        {"ip", ip}, 
        {"severity", sev},
        {"search", msg}
    };

    response["data"] = data;
    return response;
}


json CommandManager::handleGetMetrics(const json& req) {
    if (!storage) return createResponse("ERROR", "Storage neinitializat");

    int limit = req.value("limit", 50);
    string agent = req.value("agent", "ALL");  
    json data = storage->getMetrics(agent, limit);
    json response = createResponse("CONFIRMED", "Metrici recuperate");
    response["data"] = data;
    return response;
}

json CommandManager::handleGetLogs(const json& req) {
    if (!storage) return createResponse("ERROR", "Storage neinitializat");
    string ip = req.value("ip", "ALL");
    string sev = req.value("severity", "ALL");
    string msg = req.value("search", "");
    int limit = req.value("limit", 50);

    json data = storage->getLogs(ip, sev, msg, limit);
    
    json response = createResponse("CONFIRMED", "Logs retrieved");
    response["data"] = data;
    return response;
}

json CommandManager::handleGetAgentAlerts(const json& req) {
    if (!storage) return createResponse("ERROR", "Storage neinitializat");
    string agent = req.value("agent", "ALL");
    string sev = req.value("severity", "ALL");
    string msg = req.value("search", "");
    int limit = req.value("limit", 50);

    json data = storage->getAgentAlerts(agent, sev, limit);
    
    json response = createResponse("CONFIRMED", "Logs retrieved");
    response["data"] = data;
    return response;
}

json CommandManager::handleGetStats(const json& req) {
    if (!storage) return createResponse("ERROR", "Storage neinitializat");
   
    json statsData = storage->getStats();

    json response = createResponse("CONFIRMED", "Statistici generate");
    response["data"] = statsData;
    return response;
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

json CommandManager::handleLogout() {
    if(this->authenticated == false){
        return createResponse("ERROR", "Nu este autentificat");
    }
    this->authenticated = false;
    return createResponse("SUCCESS", "Deconectare reusita");
}



json CommandManager::createResponse(string status, string message) {
    json j;
    j["status"] = status;
    j["message"] = message;
    return j;
}

void CommandManager::setStorage(StorageManager* s) { 
    this->storage = s; 
}