#pragma once
#include <string>
#include <ctime>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>
#include <iostream>

using namespace std;
using json = nlohmann::json;

class LogEntry{
    string ip_source;
    string raw_text;
    string timestamp;

    //campuri UDP514
    string severity = "INFO";  // Default INFO
    string facility = "USER";  // Default USER

    //campuri TCP514 pt metrici
    bool is_metric = false;
    int cpu = 0;
    int ram = 0;
    
    // IMPORTANT: 'agent' din JSON il vom mapa peste 'hostname'
    // ca sa avem o singura variabila pentru sursa
    string hostname = "Unknown"; 

    int priority = 13;   // Default (User + Notice)
    int severity_nr = 6; // Default INFO
    int facility_nr = 1; // Default USER
    
    string app_name = "-";
    string message;

    const vector<string> severityNames = {
        "EMERGENCY", // 0
        "ALERT",     // 1
        "CRITICAL",  // 2
        "ERROR",     // 3
        "WARNING",   // 4
        "NOTICE",    // 5
        "INFO",      // 6
        "DEBUG"      // 7
    };

    const vector<string> facilityNames = {
        "KERNEL", "USER", "MAIL", "DAEMON", "AUTH", "SYSLOG", "LPR", "NEWS",
        "UUCP", "CRON", "AUTHPRIV", "FTP", "NTP", "AUDIT", "ALERT", "CLOCK",
        "LOCAL0", "LOCAL1", "LOCAL2", "LOCAL3", "LOCAL4", "LOCAL5", "LOCAL6", "LOCAL7"
    };

    void parse(){
        size_t firstChar = raw_text.find_first_not_of(" \t\n\r"); 

       //de la agent
        if(firstChar != string::npos && raw_text[firstChar] == '{'){ 
            try {
                auto j = json::parse(raw_text);

                if (j.contains("agent")) {
                    hostname = j["agent"]; 
                }

                if (j.contains("ip")) {
                    ip_source = j["ip"];
                }
                if (j.contains("msg")) 
                    message = j["msg"];
                else if (j.contains("message")) 
                    message = j["message"];
                else 
                    message = "No message data";

                if (j.contains("severity")) {
                    string s = j["severity"];
                    severity = s;
                    
                    severity_nr = 6;
                    for(size_t i=0; i<severityNames.size(); i++) {
                        if(severityNames[i] == s) {
                            severity_nr = i;
                            break;
                        }
                    }
                }

                if (j.contains("cpu")) {
                    is_metric = true;
                    cpu = j["cpu"];
                    ram = j.value("ram", 0);
                }

            } catch (...) {
                message = "Invalid JSON received: " + raw_text;
                severity = "ERROR";
                severity_nr = 3;
            }
        }
        //syslog rfc 5424
        else{ 
            size_t l_bracket = raw_text.find('<');
            size_t r_bracket = raw_text.find('>');

            if( l_bracket != string::npos && r_bracket != string::npos){
                try{
                    string pri_nr = raw_text.substr(l_bracket + 1, r_bracket - l_bracket - 1);
                    this->priority = stoi(pri_nr);

                    severity_nr = priority % 8;
                    facility_nr = priority / 8;

                    if (severity_nr >= 0 && severity_nr < 8)
                        severity = severityNames[severity_nr];
                    else 
                        severity = "UNKNOWN";

                    if (facility_nr >= 0 && facility_nr < 24) 
                        facility = facilityNames[facility_nr];
                    else 
                        facility = "UNKNOWN";
                } catch(...){
                    priority = 13; 
                }

                stringstream ss(raw_text.substr(r_bracket + 1));
                string version;
                ss >> version;
                ss >> timestamp;
                ss >> hostname;
                ss >> app_name;

                std::string procID; ss >> procID;
                std::string msgID; ss >> msgID;

                std::string temp;
                std::getline(ss, temp); 
                size_t firstContent = temp.find_first_not_of(" ");
                if(firstContent != std::string::npos) 
                    message = temp.substr(firstContent);
                else message = temp;

            } else {
            
                message = raw_text;
                severity = "INFO";
                severity_nr = 6;
            }
        }
    }

public: 
    LogEntry(string text, string ip) : raw_text(text), ip_source(ip){
        parse();
        
        if (timestamp.empty()) {
            time_t now = time(0);
            char buffer[80];
            strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&now));
            timestamp = string(buffer);
        }
    }

    LogEntry() = default;

    json toJson() const {
        return {
            {"ip_source", ip_source},
            {"agent", hostname},      
            {"severity_code", severity_nr},
            {"severity", severity},
            {"timestamp", timestamp},
            {"message", message},
            {"is_metric", is_metric},
            {"cpu", cpu},
            {"ram", ram}
        };
    }


    string getHostname() const {
         return hostname; 
    }
    int getSeverityNr() const { 
        return severity_nr; 
    }
    string getSeverity() const { 
        return severity; 
    }
    string getRawText() const { 
        return raw_text; 
    }
    string getTimestamp() const { 
        return timestamp; 
    }
    string getIp() const { 
        return ip_source; 
    }
    bool isMetric() const { 
        return is_metric; 
    }
    int getCpu() const { 
        return cpu; 
    }
    int getRam() const { 
        return ram; 
    }
    string getMsg() const { 
        return message; 
    }
};