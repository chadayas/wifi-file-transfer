#include "receiver.hpp"

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
	std::cout << "[RECV OK] Listening for file transfers on port " << RECV_PORT << std::endl;

	running = true;
	while (running){
		int peer_fd = accept(socket_fd, nullptr, nullptr);
		if (peer_fd < 0) continue;

		std::cout << "[RECV OK] Peer connected" << std::endl;

		// Read file count
		uint32_t fc_net;
		if (!recv_exact(peer_fd, &fc_net, 4)){
			close(peer_fd);
			continue;
		}
		uint32_t file_count = ntohl(fc_net);
		std::cout << "[RECV OK] Expecting " << file_count << " files" << std::endl;

		std::string dir = save_dir();
		std::filesystem::create_directories(dir);

		for (uint32_t i = 0; i < file_count; i++){
			// Read extension length
			uint32_t ext_len_net;
			if (!recv_exact(peer_fd, &ext_len_net, 4)) break;
			uint32_t ext_len = ntohl(ext_len_net);

			// Read extension
			std::string ext(ext_len, '\0');
			if (!recv_exact(peer_fd, &ext[0], ext_len)) break;

			// Read file size
			uint32_t fsize_net;
			if (!recv_exact(peer_fd, &fsize_net, 4)) break;
			uint32_t fsize = ntohl(fsize_net);

			// Read file data
			std::vector<char> data(fsize);
			if (!recv_exact(peer_fd, data.data(), fsize)) break;

			// Write to disk
			std::string filename = dir + "received_" + std::to_string(i + 1) + ext;
			std::ofstream out(filename, std::ios::binary);
			out.write(data.data(), data.size());
			out.close();

			std::cout << "[RECV OK] Saved " << filename
				<< " (" << fsize << " bytes)" << std::endl;
		}

		close(peer_fd);
	}
}

void ReceiverService::stop(){
	running = false;
	if (socket_fd >= 0) close(socket_fd);
}
