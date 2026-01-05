#pragma once
#include <vector>
#include <mutex>
#include "LogEntry.hpp"
using namespace std;

class StorageManager{
    vector<LogEntry> logs;
    mutex mtx;
public:
    void addLog(const LogEntry& entry){
        lock_guard<mutex> lock(mtx); //blochez lacatul pana se adauga
        logs.push_back(entry);
    }

    vector<LogEntry> getLogs(){
        lock_guard<mutex> lock(mtx); //blochez cat timp citesc
        return logs;
    }

    int getCount(){
        lock_guard<mutex> lock(mtx);
        return logs.size();
    }
    
};