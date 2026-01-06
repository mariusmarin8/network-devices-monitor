#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include <iostream>
using json = nlohmann::json;
#define PORT 8080

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[10000] = {0};
    char message[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Connected to server.\n");
    std::cout << "MENIU COMENZI:\n";
    std::cout << "1. Autentificare (LOGIN)\n";
    std::cout << "2. Cere Statistici (GET_STATS)\n";
    std::cout << "3. Aplica Filtru (FILTER_LOGS)\n";
    std::cout << "0. Iesire (EXIT)\n";
    std::cout << "Alege o optiune: ";
    std::string command;

    while (1) {
       std::cin>>command;
        json task;

        if(command == "LOGIN"){
            std::string user;
            std::string pass;
            std::cout << "User: "; 
            std::cin >> user;
            std::cout << "Pass: "; 
            std::cin >> pass;

            task["command"] = command;
            task["user"] = user;
            task["pass"] = pass;
        }
        else if(command == "GET_STATS"){
            task["command"] = command;
        }
        else if(command == "FILTER_LOGS"){
            std::string ip;
            std::string sev;
            std::cout << "IP Tinta: "; std::cin >> ip;
            std::cout << "Severitate: "; std::cin >> sev;    
            task["command"] = command;
            task["ip"] = ip;
            task["severity"] = sev;
        }else if (command == "GET_METRICS") {
            task["command"] = command;
        }else if (command == "GET_LOGS") {
            task["command"] = command;
        }else if(command == "LOGOUT"){
            task["command"] = command;
        }

        strcpy(message, task.dump().c_str());
        message[strcspn(message, "\n")] = 0;

        if (strcmp(message, "exit") == 0)
            break;

        send(sock, message, strlen(message), 0);

        memset(buffer, 0, sizeof(buffer));
        int bytes = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) {
            printf("Server disconnected.\n");
            break;
        }

        printf("\nServer replied: %s\n", buffer);
    }

    close(sock);
    return 0;
}

