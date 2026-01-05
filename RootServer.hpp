#pragma once
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

class RootServer
{
protected:
    int server_socket;
    int port;
    struct sockaddr_in address;
public:
    RootServer(int p);
    void initSocket(int type);
    
    //~RootServer();
};


