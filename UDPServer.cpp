#include "UDPServer.hpp"

UDPServer::UDPServer(int p) : RootServer(p){}

void UDPServer::run(){
    std::cout << "Server pornit pe UDP "<< port<<std::endl;
    initSocket(SOCK_DGRAM);
    char buffer[BUFLEN];
    while(true){
        memset(buffer, 0, BUFLEN);
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        ssize_t len = recvfrom(server_socket, buffer, BUFLEN - 1, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (len < 0) {
            perror("recvfrom failed");
            continue;
        }
        buffer[len] = '\0';

        std::cout << "Primit de la " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << ": " << buffer << std::endl;
    }
}

