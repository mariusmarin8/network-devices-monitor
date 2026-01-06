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
        
        const char* sql_logs = "CREATE TABLE IF NOT EXISTS logs ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "ip TEXT NOT NULL, "
                          "severity TEXT, "    
                          "message TEXT, "       
                          "timestamp TEXT);";

        int rc = sqlite3_exec(db, sql_logs, 0, 0, &err);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "[DB Init Error] Logs Table: %s\n", err);
            sqlite3_free(err);
        }
        
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
        char* err = nullptr; // Initializeaza cu null
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
                  entry.getRawText() + "', '" +  // Sau entry.getRawText() daca vrei mesajul brut
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

    // Returneaza ultimele 'limit' metrici de la ORICE agent
    json getMetrics(int limit = 50) {
        std::lock_guard<std::mutex> lock(mtx);
        json metricsArray = json::array();
        
        // Am scos "WHERE ip = ...". Luam tot, sortat dupa cele mai noi.
        // Adaugam si coloana 'ip' in SELECT ca sa stim sursa.
        std::string sql = "SELECT ip, cpu, ram, timestamp FROM metrics ORDER BY id DESC LIMIT ?;";
        
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, limit);

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                json point;
                // Coloana 0 e IP, 1 e CPU, etc.
                const char* ipPtr = (const char*)sqlite3_column_text(stmt, 0);
                point["ip"] = ipPtr ? ipPtr : "unknown";
                
                point["cpu"] = sqlite3_column_int(stmt, 1);
                point["ram"] = sqlite3_column_int(stmt, 2);
                
                const char* ts = (const char*)sqlite3_column_text(stmt, 3);
                point["timestamp"] = ts ? ts : "";
                
                metricsArray.push_back(point);
            }
        }
        sqlite3_finalize(stmt);
        
        // Le inversam ca sa fie cronologic (vechi -> nou)
        std::reverse(metricsArray.begin(), metricsArray.end());
        
        return metricsArray;
    }
};