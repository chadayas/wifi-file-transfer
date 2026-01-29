#ifndef GUARD_SERVER_H
#define GUARD_SERVER_H

#include "shared.hpp"
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
#include<string>
#include<thread>
#include<mutex>
#include<cstdlib>

#define TCP_PORT 8080
#define CONTENT_TYPE_STRING "Content-Type: image/"
#define CONTENT_LENGTH_STRING "Content-Length: "
#define WEBKIT_BOUNDARY_STRING "------WebKitFormBoundary"
#define HTTP_OK "HTTP/1.1 200 OK\r\n"
#define CTRL_CHARACTERS "\r\n\r\n"

enum class ParsingState {
	MULTIPART_HEADER,
	FILE_DATA,
	DONE
};

struct ParsingContext {
	ParsingState state = ParsingState::MULTIPART_HEADER;
	int file_count = 0;
	int idx_of_extensions = 0;

	std::string bytes_stash;
	std::string wbkit_bound;
	std::vector<std::string> file_extensions;
	std::ofstream current_file;
};

std::string find_boundary(const std::string &buffer);
std::string get_file_extensions(const std::string &buffer);
std::string save_to_dir();

class TCPService {
public:
	TCPService(SharedState &s) : shared(s), running(false), 
	socket_fd(-1), client_fd(-1), buffer(8196, '\0') {}
	
	~TCPService();

	void start();
	void stop();

private:
	void send_to_client(const std::string &response);
	std::string write_post();
	std::string write_response();
	std::string build_dropdown();

	void run_state_machine();
	void parse_header();
	void parse_file_data();

private:
	bool running;
	int socket_fd;
	int client_fd;
	ParsingContext ctx;
	SharedState& shared;	
	std::string buffer;
};



#endif
