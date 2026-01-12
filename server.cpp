#include<cstring>
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fstream>
#include<cctype>
#include<iomanip>
#include<iterator>
#include<sstream>

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
void print_stash(const std::string& s){
	for (std::size_t i = 0; i < s.size() ; ++i){
		unsigned char c = s[i];	
		if (c == '\r')
			std::cout << "\\r";
		else if (c == '\n')
			std::cout << "\\n";
		else if(c >= 32 && c < 127)
			std::cout << c;
		else 
			std::cout << std::hex <<"[" <<static_cast<int>(c) << "]";
	}	
	std::cout << std::endl;
	}

// fn to return the stash as a string [DEBUG]
std::string hold_stash(const std::string& s){
	std::string ret;	
	for (std::size_t i = 0; i < s.size() ; ++i){
		unsigned char c = s[i];	
		if (c == '\r')
			 ret += "\\r";
		else if (c == '\n')
			ret += "\\n";
		else if(c >= 32 && c < 127)
			ret.push_back(c);
		else {
			std::stringstream ss;
			ss << std::hex <<"[" <<static_cast<int>(c) << "]";
			ret += ss.str();	
		}
	}
	return ret;	
}





void  debug_buffer2(char buffer[]){
	std::string ct = "Content-Type: image/";	
	std::string del = "\r\n\r\n";	
	std::string str_buf(buffer);
	auto idx = str_buf.find(ct);
	auto idx_i = str_buf.find(del, idx);	
	//const char* ct = "Content-Type: image/";	
	//char* str_ptr = std::strstr(buffer, ct);
	if (idx == std::string::npos){
		std::cout << "Content-Type DNE" << std::endl;
		return;
	}
	for (std::size_t i = 0; i < str_buf.size() ; ++i){
		unsigned char c = str_buf[idx_i + i];	
		if (c == '\r')
			std::cout << "\\r";
		else if (c == '\n')
			std::cout << "\\n";
		else if(c >= 32 && c < 127)
			std::cout << c;
		else 
			std::cout << std::hex <<"[" <<static_cast<int>(c) << "]";
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
		
	std::string stash;
	int pic_bytes = recv(client_socket, buffer, sizeof(buffer), 0);
	stash.append(buffer, pic_bytes);	
	std::string buf(buffer);
	std::cout <<  std::endl <<"---This is the next buffer ----"
		<<std::endl << buf  << std::endl;	

	
	int content_length = parse_cl(buffer);
	int total_recieved = pic_bytes;	
	int file_count = 0;
	std::ofstream current_file;
	std::string wbkit_bound = find_boundary(buffer);
	State state = State::MULTIPART_HEADER;
	while (state != State::DONE){
		int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
		stash.append(buffer, bytes);	
		bool run = true;
	    while (run){
		    switch (state){
			case State::MULTIPART_HEADER:{
		 		std::string c_type = "Content-Type: image/";
				std::string ctrl_char = "\r\n\r\n";	
				auto idx = stash.find(c_type);
				auto pos = stash.find(ctrl_char, idx);		
				stash.erase(0, pos + 4);		
				file_count++;	
				std::cout <<"[State::M_H] We are on file " << file_count << "\n";	
				current_file.open("pic" + std::to_string(file_count) 
				+ ".jpeg", std::ios::out | std::ios::binary);
				
				state = State::FILE_DATA;
				run = true;	
				break;	
		 }
		    	case State::FILE_DATA:{
				auto pos = stash.find(wbkit_bound);
				if (pos != std::string::npos){
				   std::cout << " [State::FILE_DATA] We are on file " 
					   << file_count << "\n";	
				
				   current_file.write(stash.data(), pos);
		           	   stash.erase(0, pos + wbkit_bound.size());
				  	 
				   run = true;	
			           if (stash.substr(0, 2) == "--"){
					   current_file.close(); 
					   state = State::DONE;
				   } else{ 
					   current_file.close();
					   state = State::MULTIPART_HEADER; 
				   }
				} else {
				  std::size_t tail = wbkit_bound.size();
				  if (stash.size() > tail){
					  std::size_t write = stash.size() - tail;
					  current_file.write(stash.data(), write);
					  stash.erase(0, write); 
				  }
				  run = false;
				}
			   	break;
			        		
			}
			  case State::DONE:
		   		break; 
		    }
	    	if (state == State::DONE)
			break;
	    	}
	    }

	
	close(server_socket);

	return 0;
}
