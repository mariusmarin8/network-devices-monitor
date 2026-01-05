#pragma once
#include "RootServer.hpp"
#include "CommandManager.hpp"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 1024
class TCPServer : public RootServer
{
private:
    bool isDashboard;
    CommandManager command;
public:
    TCPServer(bool b, int p);
    void run();
    void setStorage(StorageManager* s) {
        command.setStorage(s);
    }
    //~TCPServer();
};

