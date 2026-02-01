#include "server.hpp"
#include "mdns.hpp"
#include "receiver.hpp"
#include <fstream>
#include <sstream>
#include <random>

namespace {
	sockaddr_in make_server(){
		sockaddr_in a{};
		a.sin_family = AF_INET;
		a.sin_port = htons(TCP_PORT);
		a.sin_addr.s_addr = INADDR_ANY;
		return a;
	}

	void parse_arp_table(SharedState& shared){
		std::ifstream arp_file("/proc/net/arp");
		if (!arp_file.is_open()) return;

		std::string line;
		std::getline(arp_file, line); // skip header

		while (std::getline(arp_file, line)){
			std::istringstream iss(line);
			std::string ip, hw_type, flags, mac, mask, device;
			if (!(iss >> ip >> hw_type >> flags >> mac >> mask >> device))
				continue;
			// Skip incomplete entries
			if (flags == "0x0") continue;

			std::string key = "arp_" + ip;
			if (shared.devices.find(key) == shared.devices.end()){
				ServiceInfo info;
				info.ip = ip;
				info.target_host = ip;
				info.port = RECV_PORT;
				info.source = DiscoverySource::ARP;
				shared.devices[key] = info;
			}
		}
	}

	std::string generate_boundary(){
		static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
		std::string boundary = "----WifiTransferBoundary";
		for (int i = 0; i < 16; i++)
			boundary += chars[dis(gen)];
		return boundary;
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
	parse_arp_table(shared);

	std::string html = "<select name=\"device\">";
	for (const auto& [name, info] : shared.devices){
		std::string label = info.target_host.empty() ? info.ip : info.target_host;
		if (info.source == DiscoverySource::ARP)
			label += " (ARP)";
		html += "<option value=\"" + name + "\">" + label + "</option>";
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

	ctx.file_buffers.emplace_back();

	ctx.state = ParsingState::FILE_DATA;
}

void TCPService::parse_file_data(){
	auto pos = ctx.bytes_stash.find(ctx.wbkit_bound);
	auto& buf = ctx.file_buffers.back();

	if (pos != std::string::npos){
		std::cout << "[State::FILE_DATA] We are on file " << ctx.file_count << "\n";

		buf.insert(buf.end(), ctx.bytes_stash.data(), ctx.bytes_stash.data() + pos);
		ctx.bytes_stash.erase(0, pos + ctx.wbkit_bound.size());

		if (ctx.bytes_stash.substr(0, 2) == "--"){
			ctx.state = ParsingState::DONE;
		} else {
			ctx.idx_of_extensions++;
			ctx.state = ParsingState::MULTIPART_HEADER;
		}
	} else {
		std::size_t tail = ctx.wbkit_bound.size();
		if (ctx.bytes_stash.size() > tail){
			std::size_t write_size = ctx.bytes_stash.size() - tail;
			buf.insert(buf.end(), ctx.bytes_stash.data(), ctx.bytes_stash.data() + write_size);
			ctx.bytes_stash.erase(0, write_size);
		}
	}
}

void TCPService::forward_to_device(const std::string& device_name){
	std::lock_guard<std::mutex> lock(shared.mtx);
	auto it = shared.devices.find(device_name);
	if (it == shared.devices.end()){
		std::cout << "[ERROR] Device not found: " << device_name << std::endl;
		return;
	}

	auto& info = it->second;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0){
		std::cout << "[ERROR] Failed to create forwarding socket" << std::endl;
		return;
	}

	sockaddr_in dest{};
	dest.sin_family = AF_INET;
	dest.sin_port = htons(info.port);
	inet_pton(AF_INET, info.ip.c_str(), &dest.sin_addr);

	if (connect(sock, (sockaddr*)&dest, sizeof(dest)) < 0){
		std::cout << "[ERROR] Failed to connect to " << info.ip << ":" << info.port << std::endl;
		close(sock);
		return;
	}

	std::cout << "[OK] Connected to " << info.ip << ":" << info.port << std::endl;

	// Build HTTP multipart POST body
	std::string boundary = generate_boundary();
	std::string body;

	for (size_t i = 0; i < ctx.file_buffers.size(); i++){
		std::string ext = (i < ctx.file_extensions.size()) ? ctx.file_extensions[i] : ".bin";
		std::string filename = "file_" + std::to_string(i + 1) + ext;

		body += "--" + boundary + "\r\n";
		body += "Content-Disposition: form-data; name=\"file[]\"; filename=\"" + filename + "\"\r\n";
		body += "Content-Type: application/octet-stream\r\n";
		body += "\r\n";
		body.append(ctx.file_buffers[i].data(), ctx.file_buffers[i].size());
		body += "\r\n";
	}
	body += "--" + boundary + "--\r\n";

	// Build HTTP request
	std::string request;
	request += "POST /upload HTTP/1.1\r\n";
	request += "Host: " + info.ip + ":" + std::to_string(info.port) + "\r\n";
	request += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
	request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	request += "Connection: close\r\n";
	request += "\r\n";
	request += body;

	// Send the full request
	size_t total_sent = 0;
	while (total_sent < request.size()){
		ssize_t n = send(sock, request.data() + total_sent, request.size() - total_sent, 0);
		if (n <= 0){
			std::cout << "[ERROR] Send failed during forwarding" << std::endl;
			close(sock);
			return;
		}
		total_sent += n;
	}

	// Read response
	std::string response(1024, '\0');
	recv(sock, &response[0], response.size(), 0);
	std::cout << "[OK] Receiver response: " << response.substr(0, response.find("\r\n")) << std::endl;

	close(sock);
	std::cout << "[OK] Forwarding complete (" << ctx.file_buffers.size() << " files via HTTP POST)" << std::endl;
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

	// Extract selected device from the first multipart part (name="device")
	auto device_key = ctx.bytes_stash.find("name=\"device\"");
	if (device_key != std::string::npos){
		auto val_start = ctx.bytes_stash.find(CTRL_CHARACTERS, device_key);
		if (val_start != std::string::npos){
			val_start += 4;
			auto val_end = ctx.bytes_stash.find(ctx.wbkit_bound, val_start);
			if (val_end != std::string::npos){
				selected_device = ctx.bytes_stash.substr(val_start, val_end - val_start);
				// trim trailing \r\n before boundary
				while (!selected_device.empty() &&
				       (selected_device.back() == '\r' || selected_device.back() == '\n'))
					selected_device.pop_back();
				std::cout << "[OK] Selected device: " << selected_device << std::endl;

				// Skip past the device part so the state machine only sees file parts
				ctx.bytes_stash.erase(0, val_end + ctx.wbkit_bound.size());
			}
		}
	}

	run_state_machine();

	if (ctx.state == ParsingState::DONE){
		auto res = write_response();
		send_to_client(res);
		std::cout << "[OK] Upload complete - " << ctx.file_count << " files" << std::endl;

		if (!selected_device.empty()){
			forward_to_device(selected_device);
		}
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
	ReceiverService receiver(shared);

	std::thread mdns_thread(&MDNSService::start, &mdns);
	std::thread recv_thread(&ReceiverService::start, &receiver);
	tcp.start();

	mdns_thread.join();
	recv_thread.join();

	return 0;
}
