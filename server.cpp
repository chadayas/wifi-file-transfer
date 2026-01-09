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
		
	return buffer + idx; 

}	

void  debug_buffer(char buffer[]){
	const char* ct = "Content-Type: image/jpeg";	
	char* str_ptr = std::strstr(buffer, ct);
	std::size_t idx = str_ptr - buffer;
	
	for (std::size_t i = 0; i < 1000; ++i){
		unsigned char p = buffer[idx+i];
		std::cout << std::hex << static_cast<int>(p) 
			<< " ";
               if (i % 10 == 0) {std::cout << std::endl;}	
	}	
	std::cout << std::endl;	

}
void find_boundary(char buffer[]){
	std::string wbkit = "------WebKitFormBoundary";	
	std::string buf_str(buffer);
	auto idx = buf_str.find(wbkit);
	std::string boundary;
		
	std::cout << std::endl;
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
	char buffer[8196] = {0};
	recv(client_socket, buffer, sizeof(buffer), 0);
	if (strncmp(buffer, "GET", 3) == 0){
		std::cout << buffer << std::endl;
		std::string response ="HTTP/1.1 200 OK\r\n";	
		std::string body ="<html>"
			"<h1>Welcome to the Wifi File Transfer!</h1><body>"
	"<form action=\"/upload\" method=\"POST\" enctype=\"multipart/form-data\">"
		"<input type=\"file\" id= \"file\" name=\"file[]\" " 
		"accept=\"image/*\" multiple>"
		"<button type=\"submit\">Submit Upload(s)</button>"
		"</form></body></html>";
		response += "Content-Type: text/html\r\n";
		response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
		response += "\r\n";
		response += body;	
		send(client_socket, response.data(), response.size(), 0);
	}	
		
	int pic_bytes = recv(client_socket, buffer, sizeof(buffer), 0);
	std::string buf(buffer);
	std::cout <<  std::endl <<"---This is the next buffer ----"
		<<std::endl << buf  << std::endl;	
	

	std::ofstream f1("pic1", std::ios::out | std::ios::binary);
	
	char* jpeg_sig_idx = trim_header(buffer);	
	int header_size = jpeg_sig_idx - buffer;
	int jpeg_sig_length = pic_bytes - header_size;
	
	f1.write(jpeg_sig_idx, jpeg_sig_length);
	int content_length = parse_cl(buffer);
	int total_recieved = pic_bytes;	
	find_boundary(buffer);

	while (total_recieved < content_length){
		int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
		f1.write(buffer, bytes);
		total_recieved += bytes;
	}

	f1.close();

	close(server_socket);

	return 0;
}
