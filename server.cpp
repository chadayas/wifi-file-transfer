#include<cstring>
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fstream>

int parse_header(const char* buffer){
	buffer.find("Content-Length:")
}


// Latptop ip 192.168.1.170
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
	recv(client_socket, buffer, sizeof(buffer), 0);
	if (strncmp(buffer, "GET", 3) == 0){
		std::cout << buffer << std::endl;
		std::string response ="HTTP/1.1 200 OK\r\n";	
		std::string body ="<html><body>"
		"<form action=\"/upload\" method=\"POST\" enctype=\"multipart/form-data\">"
		"<input type=\"file\" name=\"file\" accept=\"image/*\">"
		"<button type=\"submit\">Upload a file</button>"
		"</form></body></html>";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
		response += "\r\n";
		response += body;	
		send(client_socket, response.data(), response.size(), 0);
	}	
		
	
	int pic_bytes = recv(client_socket, buffer, sizeof(buffer), 0);	
		
	std::ofstream file("tiger.jpeg", std::ios::out | std::ios::binary);
	file.write(buffer, sizeof(buffer));
	file.close();
		
	std::cout <<  std::endl <<"---This is the next buffer ----"
		<<std::endl << buffer  << std::endl;	
	std::cout << "Photo received\n total bytes: " << pic_bytes << std::endl;	
	close(server_socket);


	return 0;
}
