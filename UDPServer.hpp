#pragma once
#include "RootServer.hpp"
#include "StorageManager.hpp"
#define BUFLEN 512
class UDPServer : public RootServer
{
    StorageManager* storage;
public:
    UDPServer(int p, StorageManager* s);
    void run();
    //~UDPServer();
};

