#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

int main()
{
	int sockfd = socket(AF_NET,SOCK_STREAM, 0);
	return 0;
}
