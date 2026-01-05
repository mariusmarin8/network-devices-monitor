#include "TCPServer.hpp"

TCPServer::TCPServer(bool b, int p) : isDashboard(b), RootServer(p){}

void TCPServer::run(){
    initSocket(SOCK_STREAM);

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    std::cout << "Serverul ascultă pe TCP " << port << std::endl;
    if (listen(server_socket, 100) < 0) { // Coada de așteptare de 100
        perror("Listen error");
        return;
    }

    int new_socket, activity, valread;
    int addrlen, sd;
    addrlen = sizeof(address);
    int client_socket[MAX_CLIENTS] = {0};
    int max_sd;
    
    char buffer[BUFFER_SIZE];
    fd_set master_set;
    fd_set read_set, write_set;

    FD_ZERO(&master_set);
    FD_SET(server_socket, &master_set);
    max_sd = server_socket;
    while(true){
        read_set = master_set;
        write_set = master_set;
        
        if (select(max_sd + 1, &read_set, NULL, NULL, NULL) == -1) { //sta blocat pana se conecteaza cineva
            perror("Select error");
            break;
        }

        if (FD_ISSET(server_socket, &read_set)) {  //daca avem un nou client, marcam socket ul
            if ((new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            
            std::cout << "New connection. SD: " << new_socket << ", IP: " << inet_ntoa(address.sin_addr) << "\n";

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }

            FD_SET(new_socket, &master_set);
            if (new_socket > max_sd) {
                max_sd = new_socket;
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];

            if (sd > 0 && FD_ISSET(sd, &read_set)) {
                
                memset(buffer, 0, BUFFER_SIZE);
                valread = read(sd, buffer, BUFFER_SIZE - 1);

                if (valread == 0) {
                    std::cout << "Host disconnected. IP: " << inet_ntoa(address.sin_addr) << "\n";
                    FD_CLR(sd, &master_set);
                    close(sd);
                    client_socket[i] = 0;
                   
                } else {
                    //verific daca este agent sau client
                   if(isDashboard){
                        std::cout << "Primt de la clientul " << sd << ": " << buffer << std::endl;
                        
                        std::string comm(buffer);
                        
                        std::string jsonReply = command.process_command(comm); 

                        send(sd, jsonReply.c_str(), jsonReply.length(), 0);
                        
                        std::cout << "[DASHBOARD] Request procesat. Raspuns trimis (" << jsonReply.length() << " bytes).\n";
                    }else{
                        std::cout << "Primt de la Agentul " << sd << ": " << buffer<<std::endl;
                        
                    }
                }
            }
        }

    }
}