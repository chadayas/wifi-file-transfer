#include "server.hpp"
#include "mdns.hpp"

namespace {
	sockaddr_in make_server(){
		sockaddr_in a{};
		a.sin_family = AF_INET;
		a.sin_port = htons(TCP_PORT);
		a.sin_addr.s_addr = INADDR_ANY;
		return a;
	}
}

std::string save_to_dir(){
	const char* env = std::getenv("HOME");
	if (!env) env = std::getenv("USERPROFILE");
	if (!env) return "./";
	
	return std::string(env) + "/Downloads/wifi_transfer/";
}

std::string find_boundary(const std::string &buf){
	std::string wbkit = WEBKIT_BOUNDARY_STRING;
	auto idx = buf.find(wbkit);
	if (idx == std::string::npos) return "";
	std::string boundary;
	for (size_t i = 0; buf[idx + i] != '\r'; ++i){
		boundary.push_back(buf[idx + i]);
	}
	return boundary;
}

std::string get_file_extensions(const std::string &buffer){
	std::string ret;
	std::string fn = "filename=";
	auto pos = buffer.find(fn);
	if (pos == std::string::npos) return "";
	auto new_pos = buffer.find('.', pos);
	if (new_pos == std::string::npos) return "";
	for (size_t i = 0; buffer[new_pos + i] != '"'; i++){
		ret.push_back(buffer[new_pos + i]);
	}
	return ret;
}


TCPService::~TCPService(){
	stop();
}
std::string TCPService::build_dropdown(){
	std::lock_guard<std::mutex> lock(shared.mtx);	
	std::string html = "<select>";
	for (const auto& [name, info ] : shared.devices){
		html += "<option value=\"" + name + "\">" + info.ip+ "</option>";
	}
	html += "</select>";	
	return html; 
}

std::string TCPService::write_post(){
	std::string response;
	response += HTTP_OK;
	std::string dropdown = build_dropdown();	
	std::string body = "<html>"
		"<h1>Welcome to the Wifi File Transfer!</h1><body>"
		"<form action=\"/upload\" method=\"POST\" enctype=\"multipart/form-data\">"
		"<label>Pick a destination</label>" + dropdown + "<br><br>"	
		"<input type=\"file\" id=\"file\" name=\"file[]\" "
		"accept=\"image/*\" multiple>"
		"<button type=\"submit\">Submit Upload(s)</button>"
		"</form></body></html>";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	response += "\r\n";
	response += body;
	return response;
}

std::string TCPService::write_response(){
	std::string response;
	response += HTTP_OK;
	std::string body = "<html>"
		"<h1>Successfully uploaded " + std::to_string(ctx.file_count) +
		" files!</h1></html>";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	response += "\r\n";
	response += body;
	return response;
}



void TCPService::send_to_client(const std::string &r){
	send(client_fd, r.data(), r.size(), 0);
}

void TCPService::parse_header(){
	auto idx = ctx.bytes_stash.find(CONTENT_TYPE_STRING);
	auto pos = ctx.bytes_stash.find(CTRL_CHARACTERS, idx);

	ctx.file_extensions.push_back(get_file_extensions(ctx.bytes_stash));
	ctx.bytes_stash.erase(0, pos + 4);
	ctx.file_count++;

	std::cout << std::string(30, '-') << "\n";
	std::cout << "[State::M_H] We are on file " << ctx.file_count << "\n";
	
	std::string home_dir = save_to_dir();
	ctx.current_file.open(home_dir + "pic" + std::to_string(ctx.file_count)
		+ ctx.file_extensions[ctx.idx_of_extensions], std::ios::out | std::ios::binary);

	ctx.state = ParsingState::FILE_DATA;
}

void TCPService::parse_file_data(){
	auto pos = ctx.bytes_stash.find(ctx.wbkit_bound);

	if (pos != std::string::npos){
		std::cout << "[State::FILE_DATA] We are on file " << ctx.file_count << "\n";

		ctx.current_file.write(ctx.bytes_stash.data(), pos);
		ctx.bytes_stash.erase(0, pos + ctx.wbkit_bound.size());

		if (ctx.bytes_stash.substr(0, 2) == "--"){
			ctx.current_file.close();
			ctx.state = ParsingState::DONE;
		} else {
			ctx.current_file.close();
			ctx.idx_of_extensions++;
			ctx.state = ParsingState::MULTIPART_HEADER;
		}
	} else {
		std::size_t tail = ctx.wbkit_bound.size();
		if (ctx.bytes_stash.size() > tail){
			std::size_t write_size = ctx.bytes_stash.size() - tail;
			ctx.current_file.write(ctx.bytes_stash.data(), write_size);
			ctx.bytes_stash.erase(0, write_size);
		}
	}
}

void TCPService::run_state_machine(){
	while (ctx.state != ParsingState::DONE){
		int bytes_amt = recv(client_fd, &buffer[0], buffer.size(), 0);
		if (bytes_amt <= 0) break;

		ctx.bytes_stash.append(buffer, 0, bytes_amt);

		bool run = true;
		while (run){
			switch (ctx.state){
				case ParsingState::MULTIPART_HEADER:
					if (ctx.bytes_stash.find(CTRL_CHARACTERS) != std::string::npos){
						parse_header();
					} else {
						run = false;
					}
					break;

				case ParsingState::FILE_DATA:
					parse_file_data();
					if (ctx.state == ParsingState::FILE_DATA){
						run = false;
					}
					break;

				case ParsingState::DONE:
					run = false;
					break;
			}
		}
	}
}

void TCPService::start(){
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0){
		std::cout << "[ERROR] Socket creation failed" << std::endl;
		return;
	}
	std::cout << "[OK] Socket created" << std::endl;

	auto tcp_addr = make_server();
	int bind_result = bind(socket_fd, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr));
	if (bind_result < 0){
		std::cout << "[ERROR] Bind failed: " << bind_result << std::endl;
		return;
	}
	std::cout << "[OK] Bind successful" << std::endl;

	listen(socket_fd, 5);
	std::cout << "[OK] Listening on port " << TCP_PORT << std::endl;

	running = true;

	client_fd = accept(socket_fd, nullptr, nullptr);
	if (client_fd < 0){
		std::cout << "[ERROR] Accept failed" << std::endl;
		return;
	}
	std::cout << "[OK] Client connected" << std::endl;

	int bytes = recv(client_fd, &buffer[0], buffer.size(), 0);
	if (bytes <= 0) return;
	if (buffer.find("GET") != std::string::npos){
		std::cout << "[OK] Received GET request" << std::endl;
		auto post = write_post();
		send_to_client(post);
	}

	bytes = recv(client_fd, &buffer[0], buffer.size(), 0);
	if (bytes <= 0) return;

	ctx.bytes_stash.append(buffer, 0, bytes);
	ctx.wbkit_bound = find_boundary(ctx.bytes_stash);

	std::cout << "[OK] Boundary: " << ctx.wbkit_bound << std::endl;

	run_state_machine();

	if (ctx.state == ParsingState::DONE){
		auto res = write_response();
		send_to_client(res);
		std::cout << "[OK] Upload complete - " << ctx.file_count << " files" << std::endl;
	}

	close(client_fd);
}

void TCPService::stop(){
	running = false;
	if (socket_fd >= 0) close(socket_fd);
}

int main(){
	SharedState shared;	
	TCPService tcp(shared);
	MDNSService mdns(shared);
	
	std::thread mdns_thread(&MDNSService::start, &mdns);
	tcp.start();
	
	mdns_thread.join();
	
	return 0;
}
