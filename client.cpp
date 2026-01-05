#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include<arpa/inet.h>
#include<string>
#include<fstream>

int main()
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(clientSocket, (struct sockaddr*)&serverAddress,
            sizeof(serverAddress));
    
    char buffer[4096] = {0};
    std::ifstream file("mj.png", std::ios::binary);
    file.read(buffer, 4096); 
    send(clientSocket, buffer, 4096, 0);
    
    close(clientSocket);

    return 0;
}
