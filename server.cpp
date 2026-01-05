#include<cstring>
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fstream>

int main()
{
	int server_socket = socket(AF_INET,SOCK_STREAM, 0);
	sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(8080);
	server_address.sin_addr.s_addr = INADDR_ANY;

	bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
	listen(server_socket, 5);
	
	int client_socket = accept(server_socket, nullptr, nullptr);
	char buffer[4096] = {0};
	int bytes_recv = recv(client_socket, buffer, sizeof(buffer), 0);
	std::ofstream file("our_gift.jpeg", std::ios::binary);
	file.write(buffer, 4096);
	std::cout << "Bytes recieved: " << bytes_recv << std::endl;
	
	close(server_socket);


	return 0;
}
