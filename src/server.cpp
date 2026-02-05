#include "server.hpp"
#include "mdns.hpp"
#include "receiver.hpp"
#include <fstream>
#include <sstream>
#include <random>
#include <netdb.h>



namespace {
	sockaddr_in make_server(){
		sockaddr_in a{};
		a.sin_family = AF_INET;
		a.sin_port = htons(TCP_PORT);
		a.sin_addr.s_addr = INADDR_ANY;
		return a;
	}

	std::string mac_vendor_lookup(const std::string& mac){
		// Common vendors - uppercase, colon-separated format expected
		std::string oui = mac.substr(0, 8);

		for (auto& c : oui) c = std::toupper(c);

		if (oui == "00:50:56" || oui == "00:0C:29" || oui == "00:15:5D") return "VMware";
		if (oui == "08:00:27") return "VirtualBox";
		if (oui.substr(0,2) == "00" || oui.substr(0,2) == "02") {
			// Check Microsoft OUIs
			if (oui == "00:15:5D" || oui == "00:1D:D8" || oui == "00:12:5A" ||
			    oui == "00:03:FF" || oui == "00:0D:3A" || oui == "28:18:78" ||
			    oui == "60:45:BD" || oui == "7C:1E:52" || oui == "98:5F:D3" ||
			    oui == "C8:3F:26" || oui == "DC:B4:C4") return "Microsoft";
		}
		// Apple
		if (oui == "00:03:93" || oui == "00:0A:95" || oui == "00:0D:93" ||
		    oui == "00:1C:B3" || oui == "00:1E:C2" || oui == "00:25:BC" ||
		    oui == "3C:15:C2" || oui == "3C:E0:72" || oui == "40:6C:8F" ||
		    oui == "44:2A:60" || oui == "60:F8:1D" || oui == "68:DB:CA" ||
		    oui == "70:DE:E2" || oui == "78:31:C1" || oui == "78:CA:39" ||
		    oui == "7C:D1:C3" || oui == "8C:85:90" || oui == "9C:20:7B" ||
		    oui == "A4:5E:60" || oui == "AC:BC:32" || oui == "B8:E8:56" ||
		    oui == "C8:69:CD" || oui == "D0:03:4B" || oui == "F0:18:98" ||
		    oui == "F8:1E:DF") return "Apple";
		// Samsung
		if (oui == "00:15:99" || oui == "00:1A:8A" || oui == "00:21:19" ||
		    oui == "00:23:39" || oui == "00:26:37" || oui == "08:D4:2B" ||
		    oui == "10:D5:42" || oui == "14:49:E0" || oui == "24:4B:81" ||
		    oui == "30:96:FB" || oui == "34:23:BA" || oui == "4C:BC:A5" ||
		    oui == "50:01:BB" || oui == "5C:F6:DC" || oui == "78:52:1A" ||
		    oui == "84:11:9E" || oui == "90:18:7C" || oui == "94:35:0A" ||
		    oui == "A8:06:00" || oui == "BC:D1:1F" || oui == "C4:73:1E" ||
		    oui == "D0:22:BE" || oui == "E4:7C:F9" || oui == "F0:25:B7" ||
		    oui == "F4:7B:5E") return "Samsung";
		// Google
		if (oui == "00:1A:11" || oui == "3C:5A:B4" || oui == "54:60:09" ||
		    oui == "94:EB:2C" || oui == "F4:F5:D8" || oui == "F4:F5:E8") return "Google";
		// Intel
		if (oui == "00:1B:21" || oui == "00:1C:C0" || oui == "00:1E:64" ||
		    oui == "00:1F:3B" || oui == "00:22:FA" || oui == "3C:A9:F4" ||
		    oui == "4C:79:6E" || oui == "5C:87:9C" || oui == "68:05:CA" ||
		    oui == "84:3A:4B" || oui == "8C:EC:4B" || oui == "A4:4C:C8" ||
		    oui == "B4:6B:FC" || oui == "E8:B1:FC") return "Intel";
		// Amazon
		if (oui == "00:FC:8B" || oui == "0C:47:C9" || oui == "18:74:2E" ||
		    oui == "40:B4:CD" || oui == "44:65:0D" || oui == "68:37:E9" ||
		    oui == "74:C2:46" || oui == "84:D6:D0" || oui == "A0:02:DC" ||
		    oui == "AC:63:BE" || oui == "B4:7C:9C" || oui == "F0:F0:A4" ||
		    oui == "FC:65:DE") return "Amazon";
		// Raspberry Pi
		if (oui == "B8:27:EB" || oui == "DC:A6:32" || oui == "E4:5F:01") return "Raspberry Pi";
		// Dell
		if (oui == "00:14:22" || oui == "00:1A:A0" || oui == "00:1E:4F" ||
		    oui == "18:03:73" || oui == "24:B6:FD" || oui == "34:17:EB" ||
		    oui == "44:A8:42" || oui == "5C:26:0A" || oui == "74:86:7A" ||
		    oui == "98:90:96" || oui == "B0:83:FE" || oui == "D4:BE:D9" ||
		    oui == "F8:B1:56") return "Dell";
		// HP
		if (oui == "00:1B:78" || oui == "00:1E:0B" || oui == "00:21:5A" ||
		    oui == "00:25:B3" || oui == "10:1F:74" || oui == "2C:41:38" ||
		    oui == "3C:D9:2B" || oui == "68:B5:99" || oui == "80:C1:6E" ||
		    oui == "94:57:A5" || oui == "B4:B5:2F" || oui == "D8:D3:85") return "HP";
		// Lenovo
		if (oui == "00:1E:4C" || oui == "00:21:5E" || oui == "00:22:68" ||
		    oui == "28:D2:44" || oui == "40:1C:83" || oui == "50:7B:9D" ||
		    oui == "6C:C2:17" || oui == "70:72:0D" || oui == "98:FA:9B" ||
		    oui == "C8:5B:76" || oui == "E8:E0:B7" || oui == "F0:03:8C") return "Lenovo";

		return "";
	}

	std::string reverse_lookup(const std::string& ip, const std::string& mac){
		sockaddr_in sa{};
		sa.sin_family = AF_INET;
		inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);

		char host[NI_MAXHOST];
		int result = getnameinfo((sockaddr*)&sa, sizeof(sa), host, sizeof(host),
		                         nullptr, 0, NI_NAMEREQD);
		if (result == 0){
			return std::string(host);
		}

		std::string vendor = mac_vendor_lookup(mac);
		if (!vendor.empty()){
			return vendor + " (" + ip + ")";
		}

		return ip;
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
				info.target_host = reverse_lookup(ip, mac);
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

	std::string read_file(const std::string& path){
		std::ifstream file(path);
		if (!file.is_open()) return "";
		std::ostringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

	const std::string FRONTEND_DIR = "frontend/";

	const char* RECEIVER_SCRIPT = R"PY(#!/usr/bin/env python3
import http.server
import os
import re

PORT = 9090
SAVE_DIR = os.path.expanduser("~/Downloads/wifi_transfer")

class UploadHandler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path != "/upload":
            self.send_response(404)
            self.end_headers()
            return

        content_length = int(self.headers.get("Content-Length", 0))
        content_type = self.headers.get("Content-Type", "")

        boundary = None
        for part in content_type.split(";"):
            part = part.strip()
            if part.startswith("boundary="):
                boundary = part[9:]
                break

        if not boundary:
            self.send_response(400)
            self.end_headers()
            return

        body = self.rfile.read(content_length)

        os.makedirs(SAVE_DIR, exist_ok=True)

        delimiter = ("--" + boundary).encode()
        parts = body.split(delimiter)
        count = 0

        for part in parts:
            if part in (b"", b"--\r\n", b"\r\n"):
                continue

            header_end = part.find(b"\r\n\r\n")
            if header_end == -1:
                continue

            headers = part[:header_end].decode(errors="replace")
            data = part[header_end + 4:]

            if data.endswith(b"\r\n"):
                data = data[:-2]

            match = re.search(r'filename="([^"]+)"', headers)
            if not match:
                continue

            filename = os.path.basename(match.group(1))
            count += 1
            path = os.path.join(SAVE_DIR, filename)
            with open(path, "wb") as f:
                f.write(data)
            print(f"Saved: {path} ({len(data)} bytes)")

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        resp = f'{{"status":"ok","files":{count}}}'
        self.send_header("Content-Length", str(len(resp)))
        self.end_headers()
        self.wfile.write(resp.encode())

print(f"Receiver listening on port {PORT}")
print(f"Files will be saved to {SAVE_DIR}")
http.server.HTTPServer(("0.0.0.0", PORT), UploadHandler).serve_forever()
)PY";
}

std::string save_to_dir(){
	const char* env = std::getenv("HOME");
	if (!env) env = std::getenv("USERPROFILE");
	if (!env) return "./";
	
	return std::string(env) + "/Downloads/wifi_transfer/";
}

std::string find_boundary(const std::string &buf){
	auto bpos = buf.find("boundary=");
	if (bpos != std::string::npos){
		bpos += 9; // skip "boundary="
		std::string boundary = "--";
		for (size_t i = bpos; i < buf.size() && buf[i] != '\r' && buf[i] != '\n'; ++i){
			boundary.push_back(buf[i]);
		}
		return boundary;
	}

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
	std::string dropdown = build_dropdown();
	std::string body = read_file(FRONTEND_DIR + "index.html");

	std::string placeholder = "{{DROPDOWN}}";
	auto pos = body.find(placeholder);
	if (pos != std::string::npos)
		body.replace(pos, placeholder.size(), dropdown);

	std::string response;
	response += HTTP_OK;
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	response += "\r\n";
	response += body;
	return response;
}

std::string TCPService::write_response(ParsingContext &ctx){
	std::string body = read_file(FRONTEND_DIR + "success.html");

	std::string placeholder = "{{FILE_COUNT}}";
	auto pos = body.find(placeholder);
	if (pos != std::string::npos)
		body.replace(pos, placeholder.size(), std::to_string(ctx.file_count));

	std::string response;
	response += HTTP_OK;
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	response += "\r\n";
	response += body;
	return response;
}



void TCPService::send_to_client(const std::string &r, int client_fd){
	send(client_fd, r.data(), r.size(), 0);
}

void TCPService::parse_header(ParsingContext &ctx){
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

void TCPService::parse_file_data(ParsingContext &ctx){
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

void TCPService::forward_to_device(const std::string& device_name, ParsingContext &ctx){
	std::lock_guard<std::mutex> lock(shared.mtx);
	auto it = shared.devices.find(device_name);
	std::string term_string = "\r\n";
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

	std::string boundary = generate_boundary();

	size_t content_length = 0;
	for (size_t i = 0; i < ctx.file_buffers.size(); i++){
		std::string ext = (i < ctx.file_extensions.size()) ? ctx.file_extensions[i] : ".bin";
		std::string filename = "file_" + std::to_string(i + 1) + ext;

		content_length += 2 + boundary.size() + 2; // --boundary\r\n
		content_length += 57 + filename.size() + 1 + 2; // Content-Disposition line + closing " + \r\n
		content_length += 38 + 2; // Content-Type: application/octet-stream\r\n + \r\n
		content_length += ctx.file_buffers[i].size();
		content_length += 2; // \r\n
	}
	content_length += 2 + boundary.size() + 4; // --boundary--\r\n

	// Send HTTP headers
	std::string request;
	request += "POST /upload HTTP/1.1\r\n";
	request += "Host: " + info.ip + ":" + std::to_string(info.port) + "\r\n";
	request += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
	request += "Content-Length: " + std::to_string(content_length) + "\r\n";
	request += "Connection: close\r\n";
	request += "\r\n";

	ssize_t n = send(sock, request.data(), request.size(), 0);
	if (n <= 0){
		std::cout << "[ERROR] Send failed during forwarding" << std::endl;
		close(sock);
		return;
	}

	// send each file part: header then data directly from buffer
	for (size_t i = 0; i < ctx.file_buffers.size(); i++){
		std::string ext = (i < ctx.file_extensions.size()) ? ctx.file_extensions[i] : ".bin";
		std::string filename = "file_" + std::to_string(i + 1) + ext;

		std::string part_header;
		part_header += "--" + boundary + "\r\n";
		part_header += "Content-Disposition: form-data; name=\"file[]\"; filename=\"" + filename + "\"\r\n";
		part_header += "Content-Type: application/octet-stream\r\n";
		part_header += "\r\n";
		send(sock, part_header.data(), part_header.size(), 0);
		send(sock, ctx.file_buffers[i].data(), ctx.file_buffers[i].size(), 0);
		send(sock, "\r\n", 2, 0);
	}
	std::string final_bound = "--" + boundary + "--\r\n";
	send(sock, final_bound.data(), final_bound.size(), 0);	

	// read response
	std::string response(1024, '\0');
	recv(sock, &response[0], response.size(), 0);
	std::cout << "[OK] Receiver response: " << response.substr(0, response.find("\r\n")) << std::endl;

	close(sock);
	std::cout << "[OK] Forwarding complete (" << ctx.file_buffers.size() << " files via HTTP POST)" 
		<< std::endl;
}

void TCPService::run_state_machine(int client_fd, std::string &buffer, ParsingContext &ctx){
	while (ctx.state != ParsingState::DONE){
		int bytes_amt = recv(client_fd, &buffer[0], buffer.size(), 0);
		if (bytes_amt <= 0) break;

		ctx.bytes_stash.append(buffer, 0, bytes_amt);

		bool run = true;
		while (run){
			switch (ctx.state){
				case ParsingState::MULTIPART_HEADER:
					if (ctx.bytes_stash.find(CTRL_CHARACTERS) != std::string::npos){
						parse_header(ctx);
					} else {
						run = false;
					}
					break;

				case ParsingState::FILE_DATA:
					parse_file_data(ctx);
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

void TCPService::serve_receiver_script(int client_fd){
	std::string script(RECEIVER_SCRIPT);
	std::string response;
	response += HTTP_OK;
	response += "Content-Type: text/x-python\r\n";
	response += "Content-Disposition: attachment; filename=\"receiver.py\"\r\n";
	response += "Content-Length: " + std::to_string(script.size()) + "\r\n";
	response += "\r\n";
	response += script;
	send_to_client(response, client_fd);
}

void TCPService::start(){
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0){
		std::cout << "[ERROR] Socket creation failed" << std::endl;
		return;
	}
	std::cout << "[OK] Socket created" << std::endl;

	int reuse = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

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

	while (running){

	int client_fd = accept(socket_fd, nullptr, nullptr);
	if (client_fd < 0) continue;

	std::thread([this, client_fd](){
		std::cout << "[OK] Client connected" << std::endl;

		ParsingContext ctx{};
		std::string selected_device;
		std::string buffer(8196, '\0');
		
		int bytes = recv(client_fd, &buffer[0], buffer.size(), 0);
		if (bytes <= 0){
			close(client_fd);
			return;
		}

		std::string request(buffer, 0, bytes);

		if (request.find("GET /receiver") != std::string::npos){
			std::cout << "[OK] Serving receiver.py download" << std::endl;
			serve_receiver_script(client_fd);
		} else if (request.find("GET /styles.css") != std::string::npos){
			std::string css = read_file(FRONTEND_DIR + "styles.css");
			std::string response;
			response += HTTP_OK;
			response += "Content-Type: text/css\r\n";
			response += "Content-Length: " + std::to_string(css.size()) + "\r\n";
			response += "\r\n";
			response += css;
			send_to_client(response, client_fd);
		} else if (request.find("GET") != std::string::npos){
			std::cout << "[OK] Serving form page" << std::endl;
			send_to_client(write_post(), client_fd);
		} else if (request.find("POST /upload") != std::string::npos){
			std::cout << "[OK] Handling file upload" << std::endl;

			// Tell Chrome to send the body now
			if (request.find("Expect: 100-continue") != std::string::npos ||
			    request.find("Expect: 100-Continue") != std::string::npos){
				std::string cont = "HTTP/1.1 100 Continue\r\n\r\n";
				send(client_fd, cont.data(), cont.size(), 0);
			}

			ctx.bytes_stash.append(request);
			ctx.wbkit_bound = find_boundary(ctx.bytes_stash);

			// Keep reading until we have the device field
			while (ctx.bytes_stash.find("name=\"device\"") == std::string::npos){
				int more = recv(client_fd, &buffer[0], buffer.size(), 0);
				if (more <= 0) break;
				ctx.bytes_stash.append(buffer, 0, more);
			}

			std::cout << "[OK] Boundary: " << ctx.wbkit_bound << std::endl;
			auto device_key = ctx.bytes_stash.find("name=\"device\"");
			if (device_key != std::string::npos){
				auto val_start = ctx.bytes_stash.find(CTRL_CHARACTERS, device_key);
				if (val_start != std::string::npos){
					val_start += 4;
					auto val_end = ctx.bytes_stash.find(ctx.wbkit_bound, val_start);
					if (val_end != std::string::npos){
						selected_device = ctx.bytes_stash.substr(val_start, val_end - 
						val_start);
						while (!selected_device.empty() &&
						       (selected_device.back() == '\r' || selected_device.back() 
							== '\n'))
							selected_device.pop_back();
						std::cout << "[OK] Selected device: " << selected_device 
						<< std::endl;
						ctx.bytes_stash.erase(0, val_end + ctx.wbkit_bound.size());
					}
				}
			}

			run_state_machine(client_fd, buffer, ctx);

			if (ctx.state == ParsingState::DONE){
				send_to_client(write_response(ctx), client_fd);
				std::cout << "[OK] Upload complete - " << ctx.file_count << " files" << std::endl;

				if (!selected_device.empty()){
					forward_to_device(selected_device, ctx);
				}
			}
		}

		close(client_fd);
	}).detach();
	} 
}
void TCPService::stop(){
	running = false;
	if (socket_fd >= 0) close(socket_fd);
}

int main(){
	std::cout << "\n  http://" << get_ip() << ":" << TCP_PORT << "\n" << std::endl;

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
