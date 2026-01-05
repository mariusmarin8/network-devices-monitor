#include "RootServer.hpp"

RootServer::RootServer(int p) : port(p), server_socket(-1){}

void RootServer::initSocket(int type){
    //creez socket
    if ((server_socket = socket(AF_INET, type, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //configurez adresa
    address.sin_family = AF_INET; //ipv4
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    //fac bind
    if (bind(server_socket, (const struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
    }
}