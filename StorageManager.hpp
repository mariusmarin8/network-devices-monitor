#pragma once
#include <vector>
#include <mutex>
#include "LogEntry.hpp"
#include <sqlite3.h>
#include <iostream>

using namespace std;
using json = nlohmann::json; 

class StorageManager{
    sqlite3* db;
    mutex mtx;

    void initDB(){
        char* err;
        //tabela users

        const char* sql_users = "CREATE TABLE IF NOT EXISTS users ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                        "username TEXT UNIQUE NOT NULL,"
                        "password TEXT NOT NULL);";

        int rc = sqlite3_exec(db, sql_users, 0, 0, &err);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "[DB Init Error] Users Table: %s\n", err);
            sqlite3_free(err);
        }   
        const char* sql_user_add = "INSERT OR IGNORE INTO users (username, password) "
                                    "VALUES ('admin', 'admin123');";

        rc = sqlite3_exec(db, sql_user_add, 0, 0, &err);
         if (rc != SQLITE_OK) {
            fprintf(stderr, "Eroare la inserare user default: %s\n", err);
            sqlite3_free(err);
        }   

        //tabela logs
        const char* sql_logs = "CREATE TABLE IF NOT EXISTS logs ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "ip TEXT NOT NULL, "
                          "severity TEXT, "    
                          "message TEXT, "       
                          "timestamp TEXT);";

        rc = sqlite3_exec(db, sql_logs, 0, 0, &err);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "[DB Init Error] Logs Table: %s\n", err);
            sqlite3_free(err);
        }
        //tabela metrics
        const char* sqlMetrics = "CREATE TABLE IF NOT EXISTS metrics ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "ip TEXT, cpu INTEGER, ram INTEGER, timestamp TEXT);";
        rc = sqlite3_exec(db, sqlMetrics, 0, 0, &err);

        if (rc != SQLITE_OK) {
            fprintf(stderr, "[DB Init Error] Metrics Table: %s\n", err);
            sqlite3_free(err);
        }
    }

public:
    StorageManager() {
        if (sqlite3_open("monitor.db", &db) != SQLITE_OK) {
            cerr << "[CRITICAL] Cannot open database: " << sqlite3_errmsg(db) << endl;
        }
        initDB();
    }

    ~StorageManager() {
        sqlite3_close(db);
    }

    void addLog(const LogEntry& entry){
        lock_guard<mutex> lock(mtx);
        char* err = nullptr; 
        string sql;

       
        if(entry.isMetric()){
            sql = "INSERT INTO metrics (ip, cpu, ram, timestamp) VALUES ('" + 
                  entry.getIp() + "', " + 
                  to_string(entry.getCpu()) + ", " + 
                  to_string(entry.getRam()) + ", '" + 
                  entry.getTimestamp() + "');";
            cout << "[DB] Insert METRIC attempt: " << entry.getIp() << endl;
        } else {
           
            sql = "INSERT INTO logs (ip, severity, message, timestamp) VALUES ('" + 
                  entry.getIp() + "', '" + 
                  entry.getSeverity() + "', '" + 
                  entry.getRawText() + "', '" +  
                  entry.getTimestamp() + "');";
            cout << "[DB] Insert LOG attempt: " << entry.getIp() << endl;
        }


        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &err);
        if (rc != SQLITE_OK) {
            cerr << "[DB Insert Error] SQL: " << sql << endl;
            cerr << "[DB Insert Error] Message: " << err << endl;
            sqlite3_free(err);
        } 
    }

    json getStats() {
        std::lock_guard<std::mutex> lock(mtx);
        json stats;
        sqlite3_stmt* stmt;


        if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM logs;", -1, &stmt, 0) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) stats["total_logs"] = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        const char* sqlErr = "SELECT COUNT(*) FROM logs WHERE severity IN ('EMERGENCY', 'ALERT', 'CRITICAL', 'ERROR');";
        if (sqlite3_prepare_v2(db, sqlErr, -1, &stmt, 0) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) stats["errors"] = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);


        const char* sqlWarn = "SELECT COUNT(*) FROM logs WHERE severity = 'WARNING';";
        if (sqlite3_prepare_v2(db, sqlWarn, -1, &stmt, 0) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) stats["warnings"] = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);

        return stats;
    }

  
    json getMetrics(string ip = "ALL", int limit = 50) {
        lock_guard<mutex> lock(mtx);
        json metrics = json::array();
    
        string sql = "SELECT ip, cpu, ram, timestamp FROM metrics WHERE 1=1";
    
        if (ip != "ALL") 
            sql += " AND ip = '" + ip + "'";
    
        sql += " ORDER BY id DESC LIMIT " + to_string(limit) + ";";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
           while (sqlite3_step(stmt) == SQLITE_ROW) {
            json m;
            m["ip"] = (const char*)sqlite3_column_text(stmt, 0);
            m["cpu"] = sqlite3_column_int(stmt, 1);
            m["ram"] = sqlite3_column_int(stmt, 2);
            m["timestamp"] = (const char*)sqlite3_column_text(stmt, 3);
            metrics.push_back(m);
        }
        }
        sqlite3_finalize(stmt);
        
        reverse(metrics.begin(), metrics.end());
        
        return metrics;
    }

    json getLogs(string ip = "ALL", string sev = "ALL", int limit = 50){
        lock_guard<mutex> lock(mtx);
        string sql = "SELECT timestamp, ip, severity, message FROM logs WHERE 1=1";
        json logs = json::array();
        if(ip != "ALL"){
            sql += " AND ip = '" + ip + "'";
        }

        if(sev != "ALL"){
            sql += " AND severity = '" + sev + "'";
        }

        sql += " ORDER BY id DESC LIMIT " + std::to_string(limit) + ";"; //ordonez desc dupa id si afisez doar 'limit' randuri
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                json log;
                log["timestamp"] = (const char*)sqlite3_column_text(stmt, 0);
                log["ip"]        = (const char*)sqlite3_column_text(stmt, 1);
                log["severity"]  = (const char*)sqlite3_column_text(stmt, 2);
                log["message"]   = (const char*)sqlite3_column_text(stmt, 3);
                logs.push_back(log);
            }
        }else {
            cerr << "Erroare SQL in getLogs " << sqlite3_errmsg(db) << endl;
        }
        sqlite3_finalize(stmt);

        reverse(logs.begin(), logs.end());
        return logs;
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
};