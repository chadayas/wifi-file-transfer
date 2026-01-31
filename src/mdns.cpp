#include "mdns.hpp"


std::string decode_name(const std::vector<unsigned char>& pkt, size_t& pos) {
	std::string name;
	while (pkt[pos] != 0) {
		if ((pkt[pos] & 0xC0) == 0xC0) {
			size_t offset = ((pkt[pos] & 0x3F) << 8) | pkt[pos + 1];
			pos += 2;
			name += decode_name(pkt, offset);
			return name;
		}
		int len = pkt[pos++];
		for (int i = 0; i < len; i++) {
			name += pkt[pos++];
		}
		name += '.';
	}
	pos++;
	if (!name.empty()) name.pop_back();
	return name;
}

void parse_response(std::vector<unsigned char>& pkt, int bytes, SharedState &s){
	if (bytes < 12) return;

	uint16_t ancount = (pkt[6] << 8) | pkt[7];
	size_t pos = 12;
	uint16_t qdcount = (pkt[4] << 8) | pkt[5];
	for (uint16_t i = 0; i < qdcount; i++) {
		while (pkt[pos] != 0) pos++;
		pos += 5;
	}

	for (uint16_t i = 0; i < ancount; i++) {
		std::string name = decode_name(pkt, pos);
		uint16_t type = (pkt[pos] << 8) | pkt[pos + 1];
		pos += 8;
		uint16_t rdlen = (pkt[pos] << 8) | pkt[pos + 1];
		pos += 2;
		size_t rdata_start = pos;

		if (type == 0x01 && rdlen == 4) { // A record
			std::string ip = std::to_string(pkt[pos]) + "." +
			                std::to_string(pkt[pos+1]) + "." +
			                std::to_string(pkt[pos+2]) + "." +
			                std::to_string(pkt[pos+3]);
			s.devices[name].ip= ip;
			//std::cout << "Found A: " << name << " -> " << ip << std::endl;
		} else if (type == 0x0C) { // PTR record
			size_t rdata_pos = rdata_start;
			std::string instance = decode_name(pkt, rdata_pos);
			//std::cout << "Found PTR: " << name << " -> " << instance << std::endl;
		}
	
		else if (type == 0x21){
			uint16_t port = (pkt[rdata_start + 4] << 8) | pkt[rdata_start + 5];
			s.devices[name].port = port; 
			//std::cout << "Found PORT " << name << " ->" << port << std::endl;	
		}
		pos = rdata_start + rdlen;
	}
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
	p.push_back(0x00); // null term	
}

namespace { // helper funcs
	
	auto make_a_record(){
		std::vector<unsigned char> a_pkt;
		a_pkt.push_back(0x00);a_pkt.push_back(0x01); // type A
		a_pkt.push_back(0x80);a_pkt.push_back(0x01); // class

		a_pkt.push_back(0x00);a_pkt.push_back(0x00); // TTL
		a_pkt.push_back(0x00);a_pkt.push_back(0x64); // TTL
		a_pkt.push_back(0x00);a_pkt.push_back(0x04); // RDlength (4 bytes for IP)

		a_pkt.push_back(0xc0);a_pkt.push_back(0xa8); // ip 192.168.
		a_pkt.push_back(0x01);a_pkt.push_back(0xaa); // 1.170

		return a_pkt;
	}

	auto make_ptr_record(){

		std::vector<unsigned char> ptr_pkt;
		
		ptr_pkt.push_back(0x00);ptr_pkt.push_back(0x0C); // type PTR
		ptr_pkt.push_back(0x80);ptr_pkt.push_back(0x01); // class

		ptr_pkt.push_back(0x00);ptr_pkt.push_back(0x00); // TTL
		ptr_pkt.push_back(0x00);ptr_pkt.push_back(0x64); // TTL

		// RDATA is the instance name
		std::vector<unsigned char> rdata;
		encode_name(rdata, LOCAL_NAME + SERVICE);

		ptr_pkt.push_back(0x00);ptr_pkt.push_back(static_cast<unsigned char>(rdata.size())); // RDlength
		for (auto b : rdata) ptr_pkt.push_back(b);

		return ptr_pkt;
	}

 	auto make_srv_record(){                                                           
		std::vector<unsigned char> srv_pkt;                                            
		srv_pkt.push_back(0x00);srv_pkt.push_back(0x21); // TYPE: SRV (33)              
		srv_pkt.push_back(0x80);srv_pkt.push_back(0x01); // CLASS: IN + cache-flush     

		srv_pkt.push_back(0x00);srv_pkt.push_back(0x00); // TTL                         
		srv_pkt.push_back(0x00);srv_pkt.push_back(0x78); // TTL = 120 sec               

		// RDATA                                                                      
		std::vector<unsigned char> rdata;                                             
		rdata.push_back(0x00);rdata.push_back(0x00); // Priority                      
		rdata.push_back(0x00);rdata.push_back(0x00); // Weight                        
		rdata.push_back(0x1F);rdata.push_back(0x90); // Port: 8080                    
		encode_name(rdata, LOCAL_NAME + "local");    // Target hostname               

		srv_pkt.push_back(0x00);srv_pkt.push_back(static_cast<unsigned                  
		char>(rdata.size()));                                                             
		for (auto b : rdata) srv_pkt.push_back(b);                                     
                                                                                    
      		return srv_pkt;                                                                
  }                                                                                 
        auto build_send_record(){
		// build header with 3 answers 	
		std::vector<unsigned char> pkt;	
		pkt.push_back(0x00);pkt.push_back(0x00); // ID                          
		pkt.push_back(0x84);pkt.push_back(0x00); // Flags (response)            
		pkt.push_back(0x00);pkt.push_back(0x00); // QDCOUNT                     
		pkt.push_back(0x00);pkt.push_back(0x03); // ANCOUNT                     
		pkt.push_back(0x00);pkt.push_back(0x00); // NSCOUNT                     
		pkt.push_back(0x00);pkt.push_back(0x00); // ARCOUNT                     
		
		auto ptr_rec = make_ptr_record();
		auto a_rec = make_a_record();
		auto srv_rec = make_srv_record();

		encode_name(pkt, SERVICE);
		pkt.insert(pkt.end(), ptr_rec.begin(), ptr_rec.end());

		encode_name(pkt, LOCAL_NAME + SERVICE);
		pkt.insert(pkt.end(), a_rec.begin(), a_rec.end());

		encode_name(pkt, LOCAL_NAME + SERVICE);
		pkt.insert(pkt.end(), srv_rec.begin(), srv_rec.end());
		
		return pkt;
	}                                


	auto make_query(){
		std::vector<unsigned char> packet;	
		packet.push_back(0x00); packet.push_back(0x00); // ID 
		packet.push_back(0x00); packet.push_back(0x00);; // Flags	
		packet.push_back(0x00); packet.push_back(0x01); // QD
		packet.push_back(0x00); packet.push_back(0x00); // AN
		packet.push_back(0x00); packet.push_back(0x00); // NS
		packet.push_back(0x00); packet.push_back(0x00); //AR

		encode_name(packet, SERVICE);
		packet.push_back(0x00);packet.push_back(0x0c); //type
		packet.push_back(0x00);packet.push_back(0x01); // class

		return packet;
	}

	sockaddr_in make_mdnsaddr(){
		sockaddr_in a{};
		a.sin_family = AF_INET;
		a.sin_port = htons(MDNS_PORT);
		a.sin_addr.s_addr = inet_addr(MDNS_IP);
	
		return a;
	}

	sockaddr_in make_bindaddr(){
		sockaddr_in a{};
		a.sin_family = AF_INET;
		a.sin_port = htons(MDNS_PORT);
		a.sin_addr.s_addr = INADDR_ANY;

		return a;
	}

	ip_mreq make_groups(){
		ip_mreq g{};
		g.imr_multiaddr.s_addr =  inet_addr(MDNS_IP);
		g.imr_interface.s_addr = INADDR_ANY;
		return g;
	}
}


MDNSService::~MDNSService(){
	stop();
}

void MDNSService::start(){
	timeval tv{1, 0};
	running = true;	
	std::vector<unsigned char> buffer(1024);
	
	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	std::cout << "Socket result: " << socket_fd << std::endl; 	
	
	auto mdns_addr = make_mdnsaddr(); 
	auto bind_addr = make_bindaddr();
	auto groups = make_groups();
	
	int opt = setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			(char *)&groups, sizeof(groups));
	std::cout << "Joining result: " << opt << std::endl; 	
	int reuse = 1;	
	int opt2 = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
	std::cout << "Reuse Port result: " << opt2 << std::endl;
	
	int opt3 = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	std::cout << "Reuse address result: " << opt3 << std::endl;
	
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	int bi = bind(socket_fd, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	std::cout << "Bind result: " << bi << std::endl; 	
	
	socklen_t size = sizeof(mdns_addr);	
	socklen_t *size_ptr = &size;	
	int ITERS = 120;	
	int i = 0;	
	while(i != ITERS){	
	int bytes = recvfrom(socket_fd, buffer.data(), buffer.size(),
			0, (struct sockaddr*)&mdns_addr, size_ptr);
		if (bytes > 0){
			std::lock_guard<std::mutex> lock(shared.mtx);
			parse_response(buffer, bytes, shared);
		} else if(bytes == -1){
			std::cout << "." << std::flush;
			send_announcement();	
			send_query();
		}	
		++i;	
	}

}

void MDNSService::stop(){
	running = false;
	if (socket_fd >= 0) close(socket_fd);
}

void MDNSService::send_announcement(){
	auto mdns_addr = make_mdnsaddr(); 
	auto record = build_send_record();
	
	sendto(socket_fd, record.data(), record.size(), 0,
		(struct sockaddr*)&mdns_addr,sizeof(mdns_addr));
}
void MDNSService::send_query(){
	auto query = make_query();	
	auto mdns_addr = make_mdnsaddr(); 
	sendto(socket_fd, query.data(), query.size(), 0,
		(struct sockaddr*)&mdns_addr,sizeof(mdns_addr));
}

/*auto MDNSService::get_devices(SharedState &s){
std::lock_guard<std::mutex> lock(shared.mtx);
}

int main(){
	SharedState shared;	
	MDNSService mdns(shared);
	mdns.start();
	std::cout << "\n";
	for (const auto& [name, info] : shared.devices){
		std::cout << name << " -> " << "ip: " << info.ip
			<< " Port: "<< info.port << std::endl;
	}	
}
*/
