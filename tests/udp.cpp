#include<sys/socket.h>
#include<iostream>
#include<string>
#include<vector>
#include<netinet/in.h>
#include<unistd.h>

#define UDP_PORT 137

auto make_netbios_pkt(){
	std::vector<unsigned char nb_pkt;
	nb_pkt.push_back(0x00);pkt.push_back(0x00); // ID                          
	nb_pkt.push_back(0x84);pkt.push_back(0x00); // Flags (response)            
	nb_pkt.push_back(0x00);pkt.push_back(0x00); // QDCOUNT                     
	nb_pkt.push_back(0x00);pkt.push_back(0x03); // ANCOUNT                     
	nb_pkt.push_back(0x00);pkt.push_back(0x00); // NSCOUNT                     
	nb_pkt.push_back(0x00);pkt.push_back(0x00); // ARCOUNT                     
	nb_pkt.push_back(0x00);a_pkt.push_back(0x01); // type A
	nb_pkt.push_back(0x80);a_pkt.push_back(0x01); // class
        
	nb_pkt.push_back(0x00);a_pkt.push_back(0x00); // TTL
	nb_pkt.push_back(0x00);a_pkt.push_back(0x64); // TTL
	nb_pkt.push_back(0x00);a_pkt.push_back(0x04); // RDlength (4 bytes for IP)
		
	


	return nb_pkt;}

void debug(int v){
	if (v < 0){
		std::cout << "Error Here";
		return;
	}else
		std::cout <<"Working";
}

int main(){
	auto u_pkt = make_netbios_pkt();
	std::vector<unsigned char> buf(8196, '\0');

	sockaddr_in u{};
	u.sin_family = AF_INET;
	u.sin_port = htons(UDP_PORT);
	u.sin_addr.s_addr = INADDR_ANY;
	
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	debug(fd);
	
	int snd = sendto(fd, u_pkt.data(), 0, (sockaddr *)&u, u_pkt.size());
	debug(snd);
		
}
