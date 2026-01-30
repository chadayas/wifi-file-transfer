#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "../src/mdns.hpp"

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


static std::vector<unsigned char> make_test_packet() {
	std::vector<unsigned char> pkt;

	// --- Header ---
	pkt.push_back(0x00); pkt.push_back(0x00); // ID
	pkt.push_back(0x84); pkt.push_back(0x00); // Flags (response)
	pkt.push_back(0x00); pkt.push_back(0x00); // QDCOUNT = 0
	pkt.push_back(0x00); pkt.push_back(0x03); // ANCOUNT = 3
	pkt.push_back(0x00); pkt.push_back(0x00); // NSCOUNT
	pkt.push_back(0x00); pkt.push_back(0x00); // ARCOUNT

	// ---- Answer 1: PTR record ----
	encode_name(pkt, SERVICE);
	pkt.push_back(0x00); pkt.push_back(0x0C);          // TYPE = PTR (12)
	pkt.push_back(0x80); pkt.push_back(0x01);          // CLASS
	pkt.push_back(0x00); pkt.push_back(0x00);
	pkt.push_back(0x00); pkt.push_back(0x64);          // TTL = 100
	std::vector<unsigned char> ptr_rdata;
	encode_name(ptr_rdata, LOCAL_NAME + SERVICE);
	pkt.push_back(0x00);
	pkt.push_back(static_cast<unsigned char>(ptr_rdata.size()));
	for (auto b : ptr_rdata) pkt.push_back(b);

	// ---- Answer 2: SRV record ----
	encode_name(pkt, LOCAL_NAME + SERVICE);
	pkt.push_back(0x00); pkt.push_back(0x21);          // TYPE = SRV (33)
	pkt.push_back(0x80); pkt.push_back(0x01);          // CLASS
	pkt.push_back(0x00); pkt.push_back(0x00);
	pkt.push_back(0x00); pkt.push_back(0x78);          // TTL = 120
	std::vector<unsigned char> srv_rdata;
	srv_rdata.push_back(0x00); srv_rdata.push_back(0x00); // Priority = 0
	srv_rdata.push_back(0x00); srv_rdata.push_back(0x00); // Weight   = 0
	srv_rdata.push_back(0x1F); srv_rdata.push_back(0x90); // Port     = 8080
	encode_name(srv_rdata, LOCAL_NAME + "local");          // Target: MyLaptop.local
	pkt.push_back(0x00);
	pkt.push_back(static_cast<unsigned char>(srv_rdata.size()));
	for (auto b : srv_rdata) pkt.push_back(b);

	// ---- Answer 3: A record ----
	encode_name(pkt, LOCAL_NAME + SERVICE);
	pkt.push_back(0x00); pkt.push_back(0x01);          // TYPE = A (1)
	pkt.push_back(0x80); pkt.push_back(0x01);          // CLASS
	pkt.push_back(0x00); pkt.push_back(0x00);
	pkt.push_back(0x00); pkt.push_back(0x64);          // TTL = 100
	pkt.push_back(0x00); pkt.push_back(0x04);          // RDLENGTH = 4
	pkt.push_back(192); pkt.push_back(168);
	pkt.push_back(1);   pkt.push_back(170);

	return pkt;
}

void parse_response(std::vector<unsigned char>& pkt, int bytes,
                    std::map<std::string,std::string>& devices) {
	if (bytes < 12) return;

	int ancount = (pkt[6] << 8) | pkt[7];
	std::cout << "AnCOunt: " << ancount << std::endl;
	size_t pos = 12;
	uint16_t qdcount = (pkt[4] << 8) | pkt[5];
	std::cout << "qdcount: " << qdcount << std::endl;
	std::cout << "Size of pkt: " << pkt.size()<< std::endl;
	for (int i = 0; i < qdcount; i++) {
		while (pkt[pos] != 0) pos++;
		pos += 5;
	}

	for (int i = 0; i < ancount; i++) {
		
		std::cout << "position before: " << pos << std::endl;
		std::string name = decode_name(pkt, pos);
		
		std::cout << "position after: " << pos << std::endl;
		uint16_t type = (pkt[pos] << 8) | pkt[pos + 1];
		std::cout << "Type: " << std::hex  << std::showbase <<type << std::endl;
		pos += 8;
		uint16_t rdlen = (pkt[pos] << 8) | pkt[pos + 1];
		std::cout << "rdlen: " <<std::dec << rdlen <<std::endl;
		pos += 2;
		size_t rdata_start = pos;

		if (type == 0x01 && rdlen == 4) { // A record
			std::string ip = std::to_string(pkt[pos]) + "." +
			                std::to_string(pkt[pos+1]) + "." +
			                std::to_string(pkt[pos+2]) + "." +
			                std::to_string(pkt[pos+3]);
			devices[ip] = name;
			std::cout << "Found A: " << name << " -> " << ip << std::endl;
		} else if (type == 0x0C) { // PTR record
			size_t rdata_pos = rdata_start;
			std::string instance = decode_name(pkt, rdata_pos);
			std::cout << "Found PTR: " << name << " -> " << instance << std::endl;
		}

		else if (type == 0x21){
			uint16_t port = (pkt[rdata_start + 4] << 8) | pkt[rdata_start + 5];	
			std::cout << port << std::endl;	
		}
		pos = rdata_start + rdlen;
	}
}

int main() {
	auto pkt = make_test_packet();
	std::map<std::string, std::string> devices;

	parse_response(pkt, static_cast<int>(pkt.size()), devices);

	return 0;
}
