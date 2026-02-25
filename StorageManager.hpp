#pragma once
#include <vector>
#include <mutex>
#include "LogEntry.hpp"
#include <sqlite3.h>
#include <iostream>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>

using namespace std;
using json = nlohmann::json; 

class StorageManager{
    sqlite3* db;
    mutex mtx;
    queue<LogEntry> logQueue;
    condition_variable cv;
    thread workerThread;
    atomic<bool> run;

    void initDB(){
        char* err;
        
        // Tabel users
        const char* sql_users = "CREATE TABLE IF NOT EXISTS users ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "username TEXT UNIQUE NOT NULL,"
                        "password TEXT NOT NULL);";

        if (sqlite3_exec(db, sql_users, 0, 0, &err) != SQLITE_OK) {
            fprintf(stderr, "[DB Error] Users: %s\n", err); sqlite3_free(err);
        }   

        const char* sql_user_add = "INSERT OR IGNORE INTO users (username, password) VALUES ('admin', 'admin123');";
        sqlite3_exec(db, sql_user_add, 0, 0, nullptr);

        // Tabel logs 
        const char* sql_logs = "CREATE TABLE IF NOT EXISTS logs ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "ip TEXT NOT NULL, "
                        "severity TEXT, "    
                        "message TEXT, "       
                        "timestamp TEXT);";

        if (sqlite3_exec(db, sql_logs, 0, 0, &err) != SQLITE_OK) {
            fprintf(stderr, "[DB Error] Logs Table: %s\n", err); sqlite3_free(err);
        }

        // Tabel metrici
        const char* sqlMetrics = "CREATE TABLE IF NOT EXISTS metrics ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                "agent TEXT, " 
                                "ip TEXT, "
                                "cpu INTEGER, "
                                "ram INTEGER, "
                                "timestamp TEXT);";
                                
        if (sqlite3_exec(db, sqlMetrics, 0, 0, &err) != SQLITE_OK) {
            fprintf(stderr, "[DB Error] Metrics Table: %s\n", err); sqlite3_free(err);
        }

        // Tabel alerte agenti
        const char* sqlAlerts = "CREATE TABLE IF NOT EXISTS agent_alerts ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                "agent TEXT, "     
                                "ip TEXT, "
                                "severity TEXT, "  
                                "message TEXT, "   
                                "timestamp TEXT);";
        
        if (sqlite3_exec(db, sqlAlerts, 0, 0, &err) != SQLITE_OK) {
            fprintf(stderr, "[DB Error] Alerts Table: %s\n", err); sqlite3_free(err);
        }
    }

    void processQueue(){
        while(run){
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [this] { return !logQueue.empty() || !run;});

            if (!logQueue.empty()) {
                LogEntry entry = logQueue.front();
                logQueue.pop();
                lock.unlock(); //deblochez mutex cat timp scriu
                writeToDB(entry); 
            }
        }
    }

    void writeToDB(const LogEntry& entry){
        sqlite3_stmt* stmt;
        string sql;
        int rc;
        string raw = entry.getRawText();

        size_t firstCharPos = raw.find_first_not_of(" \t\n\r");
        char firstChar = (firstCharPos != string::npos) ? raw[firstCharPos] : 0;
        
       
        if(entry.isMetric()){
            sql = "INSERT INTO metrics (agent, ip, cpu, ram, timestamp) VALUES (?, ?, ?, ?, datetime('now', 'localtime'));";
            
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, entry.getHostname().c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, entry.getIp().c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 3, entry.getCpu());
                sqlite3_bind_int(stmt, 4, entry.getRam());

                if(sqlite3_step(stmt) != SQLITE_DONE) {
                    cerr << "[DB Error] Metrics insert failed: " << sqlite3_errmsg(db) << endl;
                } else {
                    cout << "[DB] Metrici salvate pt: " << entry.getHostname() << endl;
                }
            }
            sqlite3_finalize(stmt);
        }
        else if (firstChar == '<' || firstChar != '{') {
            sql = "INSERT INTO logs (ip, severity, message, timestamp) VALUES (?, ?, ?, datetime('now', 'localtime'));";
            
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, entry.getIp().c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, entry.getSeverity().c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, entry.getMsg().c_str(), -1, SQLITE_STATIC); // AICI REZOLVA GHILIMELELE AUTOMAT

                if(sqlite3_step(stmt) != SQLITE_DONE) {
                    cerr << "[DB Error] Syslog insert failed: " << sqlite3_errmsg(db) << endl;
                } else {
                    cout << "[DB] Syslog UDP salvat de la: " << entry.getIp() << endl;
                }
            }
            sqlite3_finalize(stmt);
        }
   
        else {
            sql = "INSERT INTO agent_alerts (agent, ip, severity, message, timestamp) VALUES (?, ?, ?, ?, datetime('now', 'localtime'));";

            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, entry.getHostname().c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, entry.getIp().c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, entry.getSeverity().c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, entry.getMsg().c_str(), -1, SQLITE_STATIC);

                if(sqlite3_step(stmt) != SQLITE_DONE) {
                    cerr << "[DB Error] Agent Alert insert failed: " << sqlite3_errmsg(db) << endl;
                } else {
                    cout << "[DB] Alerta Agent salvata pt: " << entry.getHostname() << endl;
                }
            }
            sqlite3_finalize(stmt);
        }
    }

public:
    StorageManager() : run(true) {
        // Deschidem baza de date
        if (sqlite3_open("monitor.db", &db) != SQLITE_OK) {
            cerr << "[CRITICAL] Cannot open database: " << sqlite3_errmsg(db) << endl;
        } else {
            // Activari optionale pentru performanta
            sqlite3_exec(db, "PRAGMA journal_mode=WAL;", 0, 0, 0); 
            sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", 0, 0, 0);
        }
        
        initDB();
        workerThread = thread(&StorageManager::processQueue, this);
    }

    ~StorageManager() {
        run = false;
        cv.notify_all();
        if (workerThread.joinable()) {
            workerThread.join();
        }
        sqlite3_close(db);
    }

    void addLog(const LogEntry& entry){
        {
            lock_guard<mutex> lock(mtx);
            logQueue.push(entry);
        }
        cv.notify_one();
    }

    json getStats(string ip = "ALL") {
        lock_guard<mutex> lock(mtx);
        
        json stats;
        stats["total_logs"] = 0;
        stats["errors"] = 0;
        stats["warnings"] = 0;
        stats["info"] = 0;

        sqlite3_stmt* stmt;

        string sqlTotal = "SELECT COUNT(*) FROM logs WHERE 1=1";
        if (ip != "ALL" && !ip.empty()) sqlTotal += " AND ip = ?";
        
        if (sqlite3_prepare_v2(db, sqlTotal.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            if(ip != "ALL" && !ip.empty()) sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) stats["total_logs"] = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);


        string sqlErr = "SELECT COUNT(*) FROM logs WHERE severity IN ('EMERGENCY', 'ALERT', 'CRITICAL', 'ERROR')";
        if (ip != "ALL" && !ip.empty()) sqlErr += " AND ip = ?";

        if (sqlite3_prepare_v2(db, sqlErr.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            if(ip != "ALL" && !ip.empty()) sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) stats["errors"] = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);


        string sqlWarn = "SELECT COUNT(*) FROM logs WHERE severity = 'WARNING'";
        if (ip != "ALL" && !ip.empty()) sqlWarn += " AND ip = ?";

        if (sqlite3_prepare_v2(db, sqlWarn.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            if(ip != "ALL" && !ip.empty()) sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) stats["warnings"] = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        stats["info"] = stats["total_logs"].get<int>() - stats["errors"].get<int>() - stats["warnings"].get<int>();
        if(stats["info"].get<int>() < 0) stats["info"] = 0;

        return stats;
    }


    json getMetrics(string agent = "ALL", int limit = 50) {
        lock_guard<mutex> lock(mtx);
        string sql = "SELECT cpu, ram FROM metrics WHERE 1=1";
        if (agent != "ALL" && !agent.empty()) sql += " AND agent = ?";
        sql += " ORDER BY id DESC LIMIT ?;";
        
        sqlite3_stmt* stmt;
        double totalCpu = 0.0;
        long long totalRam = 0;
        int count = 0;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            int bindIdx = 1;
            if (agent != "ALL" && !agent.empty()) {
                sqlite3_bind_text(stmt, bindIdx++, agent.c_str(), -1, SQLITE_STATIC);
            }
            sqlite3_bind_int(stmt, bindIdx, limit);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                totalCpu += sqlite3_column_int(stmt, 0); 
                totalRam += sqlite3_column_int(stmt, 1); 
                count++;
            }
        }
        sqlite3_finalize(stmt);
        
        json result;
        if (count > 0) {
            result["avg_cpu"] = (int)(totalCpu / count);
            result["avg_ram"] = (int)(totalRam / count);
        } else {
            result["avg_cpu"] = 0;
            result["avg_ram"] = 0;
        }
        return result;
    }


    json getLogs(string ip = "ALL", string sev = "ALL", string search = "", int limit = 50){
        lock_guard<mutex> lock(mtx);
        string sql = "SELECT timestamp, ip, severity, message FROM logs WHERE 1=1";
  
        if(ip != "ALL" && !ip.empty()) sql += " AND ip = ?";
        if(sev != "ALL" && !sev.empty()) sql += " AND severity = ?";
        if(!search.empty()) sql += " AND message LIKE ?";
        sql += " ORDER BY id DESC LIMIT ?;";

        sqlite3_stmt* stmt;
        json logs = json::array();

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            int bindIdx = 1;
            if(ip != "ALL" && !ip.empty()) sqlite3_bind_text(stmt, bindIdx++, ip.c_str(), -1, SQLITE_STATIC);
            if(sev != "ALL" && !sev.empty()) sqlite3_bind_text(stmt, bindIdx++, sev.c_str(), -1, SQLITE_STATIC);
            
            if(!search.empty()) {
                string searchPattern = "%" + search + "%";
              
                sqlite3_bind_text(stmt, bindIdx++, searchPattern.c_str(), -1, SQLITE_TRANSIENT);
            }
            sqlite3_bind_int(stmt, bindIdx, limit);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                json log;
                log["timestamp"] = (const char*)sqlite3_column_text(stmt, 0);
                log["ip"]        = (const char*)sqlite3_column_text(stmt, 1);
                log["severity"]  = (const char*)sqlite3_column_text(stmt, 2);
                log["message"]   = (const char*)sqlite3_column_text(stmt, 3);
                logs.push_back(log);
            }
        }
        sqlite3_finalize(stmt);
        reverse(logs.begin(), logs.end());
        return logs;
    }


    json getAgentAlerts(string agent = "ALL", string sev = "ALL", int limit = 50){
        lock_guard<mutex> lock(mtx);
        string sql = "SELECT timestamp, agent, ip, severity, message FROM agent_alerts WHERE 1=1";
        
        if(agent != "ALL" && !agent.empty()) sql += " AND agent = ?";
        if(sev != "ALL") sql += " AND severity = ?";
        sql += " ORDER BY id DESC LIMIT ?;";

        json alerts = json::array();
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            int bindIdx = 1;
            if(agent != "ALL" && !agent.empty()) sqlite3_bind_text(stmt, bindIdx++, agent.c_str(), -1, SQLITE_STATIC);
            if(sev != "ALL") sqlite3_bind_text(stmt, bindIdx++, sev.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, bindIdx, limit);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                json alert;
                alert["timestamp"] = (const char*)sqlite3_column_text(stmt, 0);
                alert["agent"]     = (const char*)sqlite3_column_text(stmt, 1);
                alert["ip"]        = (const char*)sqlite3_column_text(stmt, 2);
                alert["severity"]  = (const char*)sqlite3_column_text(stmt, 3);
                alert["message"]   = (const char*)sqlite3_column_text(stmt, 4);
                alerts.push_back(alert);
            }
        }
        sqlite3_finalize(stmt);
        reverse(alerts.begin(), alerts.end());
        return alerts;
    }

    bool auth(string user, string pass){
        lock_guard<mutex> lock(mtx);
        string sql = "SELECT id FROM users WHERE username = ? AND password = ?;";
        sqlite3_stmt* stmt;
        bool logged = false;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, pass.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                logged = true; 
            }
        }
        sqlite3_finalize(stmt);
        return logged;
    }

    json getAgentList() {
        lock_guard<mutex> lock(mtx);
        json agents = json::array();
        const char* sql = "SELECT agent FROM metrics WHERE agent IS NOT NULL AND agent != '' "
                          "UNION "
                          "SELECT agent FROM agent_alerts WHERE agent IS NOT NULL AND agent != '' "
                          "ORDER BY 1 ASC;";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* name = (const char*)sqlite3_column_text(stmt, 0);
                if(name) agents.push_back(name);
            }
        }
        sqlite3_finalize(stmt);
        return agents;
    }

    json getSyslogIPs() {
        lock_guard<mutex> lock(mtx);
        json ips = json::array();
        const char* sql = "SELECT DISTINCT ip FROM logs WHERE ip IS NOT NULL ORDER BY ip ASC;";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* ip = (const char*)sqlite3_column_text(stmt, 0);
                if(ip) ips.push_back(ip);
            }
        }
        sqlite3_finalize(stmt);
        return ips;
    }
};