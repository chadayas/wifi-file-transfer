#include<cstring>
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fstream>
#include<cctype>
#include<iomanip>

int parse_cl(char buffer[]){
	const char* cl = "Content-Length: ";	
	char* str_ptr = std::strstr(buffer, cl);
	std::size_t idx = str_ptr - buffer;
	idx += 16;
	std::size_t i = 0;	
	std::string cl_str;	
	while (!std::isspace(buffer[idx + i])){
		char char_nums = buffer[idx + i];	
		cl_str.push_back(char_nums);	
		++i;	
	}
	int cl_int = std::stoi(cl_str);
	
	return cl_int;
}

char* trim_header(char buffer[]){
	const char* ct = "Content-Type: image/jpeg";	
	char* str_ptr = std::strstr(buffer, ct);
	std::size_t idx = str_ptr - buffer;
	idx += 28;
		
	return buffer[idx]; 

}	


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

	std::cout <<  std::endl <<"---This is the next buffer ----"
		<<std::endl << buffer  << std::endl;	
	
	std::ofstream file("tiger.jpeg", std::ios::out | std::ios::binary);
	
	char* jpeg_sig_idx = trim_header(buffer)	
	int header_size = jpeg_sig_idx - buffer;
	int jpeg_sig_length = pic_bytes - header_size;
	
	file.write(jpeg_sig_idx, jpeg_sig_length);
	int content_length = parse_cl(buffer);
	int total_recieved = pic_bytes;	
	
	/*while (total_recieved < content_length){
		int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
		file.write(buffer, bytes);
		total_recieved += bytes;
	}*/

	file.close();

	close(server_socket);

	return 0;
}
