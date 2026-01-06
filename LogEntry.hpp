#pragma once
#include <string>
#include <ctime>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>
using namespace std;
using json = nlohmann::json;
// <PRI>VERSION TIMESTAMP HOSTNAME APP-NAME PROCID MSGID STRUCTURED-DATA MSG |||||       PRI = Facility × 8 + Severity

class LogEntry{
    string ip_source;
    string raw_text;
    string timestamp;

    //campuri UDP514
    string severity;  //sev = pri % 8
    string facility;  //fac = pri / 8 (este cine a trimis notificarea)

    //campuri TCP514 pt metrici
    bool is_metric = false;
    int cpu = 0;
    int ram = 0;

    int priority = 0;
    int severity_nr = 1; //info
    int facility_nr = 6; //user
    string hostname;
    string app_name;
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
        size_t firstChar = raw_text.find_first_not_of(" \t\n\r"); //curat spatiile libere

        if(firstChar != string::npos && raw_text[firstChar] == '{'){ //este json
            try {
                auto j = json::parse(raw_text);
                if (j.contains("cpu")) {
                    is_metric = true;
                    cpu = j["cpu"];
                    ram = j.value("ram", 0);
                    message = j.value("msg", "Metric data");
                }
            } catch (...) {
                message = "Invalid JSON received";
            }
        }else{ //este syslog
            size_t l_bracket = raw_text.find('<');
        size_t r_bracket = raw_text.find('>');

        if( l_bracket != string::npos && r_bracket != string::npos){
            try{
                string pri_nr = raw_text.substr(l_bracket + 1, r_bracket - l_bracket - 1);
                this->priority = stoi(pri_nr);

                severity_nr= priority % 8;
                facility_nr = priority / 8;

               
                if (severity_nr >= 0 && severity_nr < 8)
                    severity = severityNames[severity_nr];
                else severity = "UNKNOWN";

                if (facility_nr >= 0 && facility_nr < 24) 
                    facility = facilityNames[facility_nr];
                else facility = "UNKNOWN";
                }catch(...){
                    priority = 13; // user notice
                }

                stringstream ss(raw_text.substr(r_bracket + 1));
                string version;
                ss >> version;
                ss >> timestamp;
                ss >> hostname;
                ss >> app_name;

                std::string procID; 
                ss >> procID;

                std::string msgID; 
                ss >> msgID;

                std::string temp;
                std::getline(ss, temp); 
                size_t firstChar = temp.find_first_not_of(" ");

                if(firstChar != std::string::npos) 
                    message = temp.substr(firstChar);
                else message = temp;

            }else{
                message = raw_text;
                severity = "INFO";
            }
        }
    }
public: 
    LogEntry(string text, string ip) : raw_text(text), ip_source(ip){
        parse();
        if (timestamp.empty()) {
            time_t now = time(0);
            timestamp = ctime(&now);
            timestamp.pop_back(); // elimin \n de la ctime
        }
    }

    LogEntry() = default;

    
    json toJson() const {
        return {
            {"ip_source", ip_source},
            {"priority", priority},
            {"severity_code", severity_nr},
            {"severity", severity},
            {"facility_code", facility_nr},
            {"facility", facility},
            {"timestamp", timestamp},
            {"hostname", hostname},
            {"app_name", app_name},
            {"message", message},
            {"raw", raw_text}
        };
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
