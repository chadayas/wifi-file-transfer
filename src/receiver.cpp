#include "receiver.hpp"
#include "server.hpp"

ReceiverService::~ReceiverService(){
	stop();
}

std::string ReceiverService::save_dir(){
	const char* env = std::getenv("HOME");
	if (!env) env = std::getenv("USERPROFILE");
	if (!env) return "./";
	return std::string(env) + "/Downloads/wifi_transfer/";
}

bool ReceiverService::recv_exact(int fd, void* buf, size_t len){
	size_t total = 0;
	while (total < len){
		ssize_t n = recv(fd, (char*)buf + total, len - total, 0);
		if (n <= 0) return false;
		total += n;
	}
	return true;
}

std::string ReceiverService::recv_all(int fd){
	std::string data;
	char buf[8192];

	// First, read until we have the full headers
	while (true){
		ssize_t n = recv(fd, buf, sizeof(buf), 0);
		if (n <= 0) break;
		data.append(buf, n);
		if (data.find("\r\n\r\n") != std::string::npos) break;
	}

	// Parse Content-Length from headers
	size_t cl_pos = data.find("Content-Length: ");
	if (cl_pos == std::string::npos) return data;

	size_t cl_end = data.find("\r\n", cl_pos);
	size_t content_length = std::stoul(data.substr(cl_pos + 16, cl_end - cl_pos - 16));

	size_t header_end = data.find("\r\n\r\n") + 4;
	size_t body_so_far = data.size() - header_end;

	// Read remaining body
	while (body_so_far < content_length){
		ssize_t n = recv(fd, buf, sizeof(buf), 0);
		if (n <= 0) break;
		data.append(buf, n);
		body_so_far += n;
	}

	return data;
}

void ReceiverService::handle_client(int peer_fd){
	std::string data = recv_all(peer_fd);
	if (data.empty()){
		close(peer_fd);
		return;
	}

	// Check it's a POST to /upload
	if (data.find("POST /upload") == std::string::npos){
		std::string resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
		send(peer_fd, resp.data(), resp.size(), 0);
		close(peer_fd);
		return;
	}

	// Find the boundary from Content-Type header
	std::string boundary = find_boundary(data);
	if (boundary.empty()){
		// Try standard boundary format: boundary=XXXX
		size_t bpos = data.find("boundary=");
		if (bpos != std::string::npos){
			size_t bstart = bpos + 9;
			size_t bend = data.find("\r\n", bstart);
			boundary = "--" + data.substr(bstart, bend - bstart);
		}
	}

	if (boundary.empty()){
		std::cout << "[RECV ERROR] No boundary found" << std::endl;
		std::string resp = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
		send(peer_fd, resp.data(), resp.size(), 0);
		close(peer_fd);
		return;
	}

	std::cout << "[RECV OK] Boundary: " << boundary << std::endl;

	// Find body start (after \r\n\r\n)
	size_t body_start = data.find("\r\n\r\n");
	if (body_start == std::string::npos){
		close(peer_fd);
		return;
	}
	body_start += 4;

	std::string body = data.substr(body_start);
	std::string dir = save_dir();
	std::filesystem::create_directories(dir);

	int file_count = 0;
	size_t pos = 0;

	while (true){
		// Find next boundary
		size_t part_start = body.find(boundary, pos);
		if (part_start == std::string::npos) break;
		part_start += boundary.size();

		// Check for final boundary --
		if (body.substr(part_start, 2) == "--") break;

		// Skip past \r\n after boundary
		part_start += 2;

		// Find the end of part headers
		size_t header_end = body.find("\r\n\r\n", part_start);
		if (header_end == std::string::npos) break;

		std::string part_header = body.substr(part_start, header_end - part_start);
		header_end += 4; // skip \r\n\r\n

		// Extract filename extension
		std::string ext = get_file_extensions(part_header);
		if (ext.empty()) ext = ".bin";

		// Find the next boundary to determine data end
		size_t data_end = body.find(boundary, header_end);
		if (data_end == std::string::npos) break;

		// data_end - 2 to exclude the \r\n before boundary
		size_t file_data_end = data_end - 2;
		size_t file_size = file_data_end - header_end;

		// Save file
		file_count++;
		std::string filename = dir + "received_" + std::to_string(file_count) + ext;
		std::ofstream out(filename, std::ios::binary);
		out.write(body.data() + header_end, file_size);
		out.close();

		std::cout << "[RECV OK] Saved " << filename
			<< " (" << file_size << " bytes)" << std::endl;

		pos = data_end;
	}

	std::cout << "[RECV OK] Received " << file_count << " files via HTTP POST" << std::endl;

	std::string resp_body = "{\"status\":\"ok\",\"files\":" + std::to_string(file_count) + "}";
	std::string resp = "HTTP/1.1 200 OK\r\n";
	resp += "Content-Type: application/json\r\n";
	resp += "Content-Length: " + std::to_string(resp_body.size()) + "\r\n";
	resp += "\r\n";
	resp += resp_body;
	send(peer_fd, resp.data(), resp.size(), 0);

	close(peer_fd);
}

void ReceiverService::start(){
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0){
		std::cout << "[RECV ERROR] Socket creation failed" << std::endl;
		return;
	}

	int reuse = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(RECV_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(socket_fd, (sockaddr*)&addr, sizeof(addr)) < 0){
		std::cout << "[RECV ERROR] Bind failed" << std::endl;
		return;
	}

	listen(socket_fd, 5);
	std::cout << "[RECV OK] HTTP receiver listening on port " << RECV_PORT << std::endl;

	running = true;
	while (running){
		int peer_fd = accept(socket_fd, nullptr, nullptr);
		if (peer_fd < 0) continue;

		std::cout << "[RECV OK] Peer connected" << std::endl;
		handle_client(peer_fd);
	}
}

void ReceiverService::stop(){
	running = false;
	if (socket_fd >= 0) close(socket_fd);
}
