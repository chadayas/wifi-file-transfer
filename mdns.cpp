#include<sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include<iostream>
#include<sys/time.h>

int make_query(char *packet){
	unsigned char packet[512];
	packet[0] = 0x13; 	
	packet[1] = 0x00; // ID 
	
	packet[2] = 0x01; 
	packet[3] = 0x00; // Flags	
	
	packet[4] = 0x00;
	packet[5] = 0x01; // QD
	
	packet[6] = 0x00;
	packet[7] = 0x00; // AN
	
	packet[8] = 0x00;
	packet[9] = 0x00; // NS
	
	packet[10] = 0x00; 
	packet[11] = 0x00; //AR
	std::string question = "Where_Photo_Transfer?";
	unsigned char str_data = question.data();

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
	char buffer[1024] = {0};

	setsockopt(socket_test, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			(char *)&group, sizeof(group));
	
	bind(socket_test, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
	
	setsockopt(socket_test, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	int bytes = recvfrom(socket_test, buffer, sizeof(buffer), 0,
		(struct sockaddr*)&group_sock, &group_len);
	if (bytes < 0)
		std::cout << "[ERROR:SERVER] NO DATA RECIEVED" << std::endl;
	else
		std::cout << "[WORKING:SERVER] GOOD RECIEVE" << std::endl;

	int send_fn = sendto(socket_test, buffer, sizeof(buffer),
		0, (struct sockaddr*)&group_sock, sizeof(group_sock));
	if (send_fn < 0){
		std::cout << "[ERROR] SENDING ERROR" << std::endl;
	} else{
		std::cout << "[WORKING:SERVER] MESSAGE SENT.....(buffer): " << 
			buffer << std::endl;
	}
	
	return 0;
}
