#pragma once
#include "RootServer.hpp"
#define BUFLEN 512
class UDPServer : public RootServer
{

public:
    UDPServer(int p);
    void run();
    //~UDPServer();
};

