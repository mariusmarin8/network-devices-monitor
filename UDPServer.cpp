#include "UDPServer.hpp"

UDPServer::UDPServer(int p, StorageManager* s) : RootServer(p), storage(s){}

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
        string ip = inet_ntoa(client_addr.sin_addr);
        LogEntry entry(buffer, ip);

        if(storage) {
            storage->addLog(entry);
        }
        cout << "[UDP] Log salvat: " << entry.getSeverity() << " de la " << ip << std::endl;
    }
}

