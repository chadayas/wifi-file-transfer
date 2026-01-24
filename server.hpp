#ifndef GUARD_SERVER_H
#define GUARD_SERVER_H
#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fstream>
#include<cctype>
#include<iomanip>
#include<iterator>
#include<sstream>
#include<vector>
 
#define TCP_PORT 8080
#define CONTENT_TYPE_STRING "Content-Type: image/"
#define CONTENT_LENGTH_STRING "Content-Lgenth: "
#define WEBKIT_BOUNDARY_STRING "------WebKitFormBoundary"
#define HTTP_OK "HTTP/1.1 200 OK\r\n"

enum class ParsingState {
         MULTIPART_HEADER,
         FILE_DATA,
         DONE
 };

struct Context {
	ParsingState p_state = ParsingState::MULTIPART_HEADER;
	
}

int parse_context_length(const std::string &buffer);
std::string find_boundary(const std::string &buffer);
std::string get_file_extensions(const std::string &buffer);

class TCPService {
	public:
		TCPService();
		~TCPService();
		
		void start();
		void stop();
		void send_to_client(const std::string &response);
		
		bool running;
		int socket_fd;		
		int client_fd;	
	private:
		std::string write_post();
		std::string write_response();

		int file_count{}, idx_of_extenstions{}, start_post{};
		std::string bytes_stash;	
		std::vector<std::string> file_extensions;
};



#endif
