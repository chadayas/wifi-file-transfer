#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>
#include<sys/time.h>
#include<vector>

// we pass "_filetransfer._tcp.local" to name parse in order to get length
void encode_name(std::vector<unsigned char> &p, const std::string name){
	auto start = 0;
	
}

auto make_query(){
	packet.push_back(0x13); 	
	packet.push_back(0x00); // ID 
	
	packet.push_back(0x00); 
	packet.push_back(0x00);; // Flags	
	
	packet.push_back(0x00);
	packet.push_back(0x01); // QD
	
	
	packet.push_back(0x00);
	packet.push_back(0x00); // AN
	
	
	packet.push_back(0x00); 
	packet.push_back(0x00); // NS
	
	packet.push_back(0x00); 
	packet.push_back(0x00); //AR
	
	std::string question = "Where_Photo_Transfer?";
	for (const auto &x : question)
		packet.push_back(x);	
		
	return packet;
}

template <typename T>
void print(T a){
	for (const auto& x : a)
		std::cout << x<< " ";
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
	group_sock.sin_port = htons(4321);
	group_sock.sin_addr.s_addr = inet_addr("225.1.1.1");
	
	socklen_t group_len = sizeof(group_sock);	
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(4321);
	bind_addr.sin_addr.s_addr = INADDR_ANY;

	group.imr_multiaddr.s_addr =  inet_addr("225.1.1.1");
	group.imr_interface.s_addr = INADDR_ANY;
	
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	std::vector<char> buffer(1024);

	setsockopt(socket_test, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			(char *)&group, sizeof(group));
	
	bind(socket_test, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	
	setsockopt(socket_test, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	int bytes = recvfrom(socket_test, buffer.data(), buffer.size(), 0,
		(struct sockaddr*)&group_sock, &group_len);
	if (bytes < 0)
		std::cout << "[ERROR:SERVER] NO DATA RECIEVED" << std::endl;
	else
		std::cout << "[WORKING:SERVER] GOOD RECIEVE" << std::endl;

	int send_fn = sendto(socket_test, buffer.data(), buffer.size(),
		0, (struct sockaddr*)&group_sock, sizeof(group_sock));
	if (send_fn < 0){
		std::cout << "[ERROR] SENDING ERROR" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] MESSAGE SENT..... " << 
			 std::endl;
		std::cout << "Buffer: ";	
		print(buffer);
	}
	
	return 0;
}
