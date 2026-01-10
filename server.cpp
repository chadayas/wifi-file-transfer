#include<cstring>
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fstream>
#include<cctype>
#include<iomanip>

enum class State {
	MULTIPART_HEADER,
	FILE_DATA,
	DONE
};



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


	
//debug fn
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

std::string find_boundary(char buffer[]){
	std::string wbkit = "------WebKitFormBoundary";	
	std::string buf_str(buffer);
	auto idx = buf_str.find(wbkit);
	std::string boundary;
	for (auto i = 0; buffer[idx + i] != '\r'; ++i){
		boundary.push_back(buffer[idx+ i]);
	}	
	return boundary;
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
	

	std::ofstream f1("pic1.jpeg", std::ios::out | std::ios::binary);
	

	int content_length = parse_cl(buffer);
	int total_recieved = pic_bytes;	
	std::string wbkit_bound = find_boundary(buffer);
	State state;
	std::string stash;
	while (state != State::DONE){
		int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
		stash.append(buffer, bytes);	
		bool run;
	    while (run){
		    switch (state){
			case State::MULTIPART_HEADER:{
		 	std::string c_type = "Content-Type: image/jpeg";
			auto idx = stash.find(c_type);
			stash.erase(0, idx + c_type.size() )		
			state = State::FILE_DATA;
			break;	
		 }
		    	case State::FILE_DATA:{
			std::string wbkit = find_boundary(buffer);
			auto pos = stash.find(wbkit);
			if (pos != std::string::npos){
			   f1.write(stash.data(), pos);
		           stash.erase(0, pos + wbkit.size());

			    }		
			}

		    
			case State::DONE:
		    }
	    }

	}

	f1.close();

	close(server_socket);

	return 0;
}
