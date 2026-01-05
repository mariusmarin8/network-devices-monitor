#pragma once
#include <vector>
#include <mutex>
#include "LogEntry.hpp"
#include <sqlite3.h>
using namespace std;

class StorageManager{
    sqlite3* db;
    mutex mtx;

    void initDB(){
        char* errMsg;
        const char* sql = "CREATE TABLE IF NOT EXISTS logs ("
                          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                          "ip TEXT NOT NULL, "
                          "raw_text TEXT NOT NULL, "
                          "timestamp TEXT);";

        int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Eroare SQL: %s\n", errMsg);
            sqlite3_free(errMsg);
        }
    }
public:

    StorageManager(){
        int rc = sqlite3_open("logs.db", &db);
        if(rc){
            cerr << "Nu pot deschide baza de date: "
                  << sqlite3_errmsg(db) << std::endl;
        }else{
            initDB();
        }
    }

    ~StorageManager() {
        sqlite3_close(db);
    }
    void addLog(const LogEntry& entry){
        lock_guard<mutex> lock(mtx); //blochez lacatul pana se adauga
        char* err;
        string sql = "INSERT INTO logs (ip, raw_text, timestamp) VALUES ('" + entry.getIp() + "', '" + entry.getRawText() + "', '" + entry.getTimestamp() + "');";
        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &err);
        if (rc != SQLITE_OK) {
            
            fprintf(stderr, "SQL insert error: %s\n", err);
            sqlite3_free(err);
        }
    }

    vector<LogEntry> getLogs(){
        lock_guard<mutex> lock(mtx); //blochez cat timp citesc
        sqlite3_stmt* stmt;
        vector<LogEntry> result;
        const char* sql = "SELECT ip, raw_text FROM logs ORDER BY id DESC";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK){
            while(sqlite3_step(stmt) == SQLITE_ROW){
                const char* ip = (const char*)sqlite3_column_text(stmt, 0);
                const char* raw = (const char*)sqlite3_column_text(stmt, 1);

                if(ip && raw)
                    result.emplace_back(string(raw), string(ip));
            }
        }

        sqlite3_finalize(stmt); 
        return result;
    }


};