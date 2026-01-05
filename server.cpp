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
#include <thread>
#include <nlohmann/json.hpp>

#define BUFFER_SIZE 1024
#include "UDPServer.hpp"
#include "TCPServer.hpp"

int main(int argc, char *argv[]) {
    UDPServer udp(514);

    TCPServer dash(true, 8080);
    TCPServer agent(false, 514);
    std::thread t1(&UDPServer::run, &udp); 
    std::thread t2(&TCPServer::run, &dash); 
    std::thread t3(&TCPServer::run, &agent);

    t1.join(); 
    t2.join();
    t3.join();
    return 0;
}