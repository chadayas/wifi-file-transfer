#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>
#include<sys/time.h>
#include<vector>


void parse_query(std::vector<unsigned char> &t){
	int MAX = 50;	
	for (size_t i = 0; i < MAX; ++i)
		std::cout << std::showbase << std::hex << static_cast<int>(t[i]) << " " <<  std::dec;
	std::cout << "\n";
}

void encode_name(std::vector<unsigned char> &p, const std::string &name){
	size_t start = 0;
	size_t pos;	
	while ((pos = name.find('.', start)) != std::string::npos){
		size_t len = pos - start;
		p.push_back(static_cast<unsigned char>(len));
		for (size_t i = start; i < pos; ++i){
			p.push_back(name[i]);
		}
		start = pos + 1;	
	}
	size_t len = name.size() - start;
	p.push_back(static_cast<unsigned char>(len));
	for (size_t i = start; i < name.size(); ++i){
		p.push_back(name[i]);
	}
	p.push_back(0x00);
}

auto make_call(){
	std::vector<unsigned char> packet;
	packet.push_back(0x00);packet.push_back(0x00);//ID
	packet.push_back(0x84);packet.push_back(0x00); // Flags
	packet.push_back(0x00);packet.push_back(0x00);// QDCOUNT
	packet.push_back(0x00);packet.push_back(0x01); // ANCOUNT
	packet.push_back(0x00);packet.push_back(0x00); // NSCOUNT 
	packet.push_back(0x00);packet.push_back(0x00); // ARCOUNT
	
	encode_name(packet, "MyLaptop._phototransfer._tcp.local"); // name
	packet.push_back(0x00);packet.push_back(0x01); // type
	packet.push_back(0x00);packet.push_back(0x01); // class 
	packet.push_back(0x00);packet.push_back(0x64); // TTL
	packet.push_back(0x00);packet.push_back(0x04); //RDlength
	
	packet.push_back(0xc0);packet.push_back(0xa8); // ip
	packet.push_back(0x01);packet.push_back(0xaa);
	packet.push_back(0x00); // null term	
	return packet;
}

auto make_query(){
	std::vector<unsigned char> packet;	
	packet.push_back(0x00); packet.push_back(0x00); // ID 
	packet.push_back(0x00); packet.push_back(0x00);; // Flags	
	packet.push_back(0x00); packet.push_back(0x01); // QD
	packet.push_back(0x00); packet.push_back(0x00); // AN
	packet.push_back(0x00); packet.push_back(0x00); // NS
	packet.push_back(0x00); packet.push_back(0x00); //AR
	
	encode_name(packet, "_filetransfer._tcp.local");
	packet.push_back(0x01);
	packet.push_back(0x01);
	return packet;
}

template <typename T>
void print(T a){
	for (const auto& x : a)
	std::cout << x;
	std::cout << std::endl;
}

int main(){
	
	sockaddr_in group_sock;
	sockaddr_in bind_addr;	
	in_addr mul_ips;
	ip_mreq group;
	timeval tv;	
	
	int socket_test = socket(AF_INET, SOCK_DGRAM, 0);
	if ( socket_test < 0){
		std::cout << "[ERROR] Socket connection BAD" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] SOCKET IS GOOD....." << std::endl;
	}
	
	group_sock.sin_family = AF_INET;
	group_sock.sin_port = htons(5353);
	group_sock.sin_addr.s_addr = inet_addr("224.0.0.251");
	
	socklen_t group_len = sizeof(group_sock);	
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(5353);
	bind_addr.sin_addr.s_addr = INADDR_ANY;

	group.imr_multiaddr.s_addr =  inet_addr("224.0.0.251");
	group.imr_interface.s_addr = INADDR_ANY;
	
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	std::vector<unsigned char> buffer(1024);
	
	int opt = setsockopt(socket_test, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			(char *)&group, sizeof(group));
	std::cout << "Joining result: " << opt << std::endl; 	
	int reuse = 1;	
	int opt2 = setsockopt(socket_test, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
	std::cout << "Reuse Port result: " << opt2 << std::endl;
	
	int opt3 = setsockopt(socket_test, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	std::cout << "Reuse address result: " << opt3 << std::endl;
	
	int bi = bind(socket_test, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	std::cout << "Bind result: " << bi << std::endl; 	
	
	setsockopt(socket_test, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	socklen_t size = sizeof(group_sock);	
	socklen_t *size_ptr = &size;	
	
	bool running = true;	
	
	auto call = make_call();
	auto query = make_query();

	int send = sendto(socket_test, call.data(), call.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));
	std::cout << "[SERVER] Send announcement result: " << send << std::endl;	
	
	int send2 = sendto(socket_test, query.data(), query.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));
	std::cout << "[SERVER] Send query result: " << send2 << std::endl;	

	while(running){	
	int bytes= recvfrom(socket_test, buffer.data(), buffer.size(),
		0, (struct sockaddr*)&group_sock, size_ptr);
		if (bytes > 0){
			std::cout << "[recieve working] : ";
			print(buffer);
			std::cout << "\n";	
		}else if(bytes == -1){
			std::cout << "." << std::flush;
			sendto(socket_test, call.data(), call.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));	
			
			sendto(socket_test, query.data(), query.size(), 0,
			(struct sockaddr*)&group_sock,sizeof(group_sock));
	
		} 
			
	}
	return 0;
}
