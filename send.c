#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "progress_bar.h"

#define PORT 8888
#define TIMEOUT_USEC 900000
#define MAXFILE 50

struct ping_header
{
	char name[50];
	char filename[50];
	struct stat fileinfo;
};

void zero_out(struct sockaddr_in *ad);
void ping(struct ping_header *header, struct sockaddr_in *addr, char *server_name, int size);
void check(int n, char *str);
int set_time_out(int sockfd, long int usecs);

int main (int argc, char *argv[])
{
	if (argc != 2){
		puts("Usage: sendfile <filename>");
		return 1;
	}

	char filename[MAXFILE];
	strncpy(filename, argv[1], MAXFILE);

	struct ping_header header;
	gethostname(header.name, sizeof(header.name));
	strncpy((header.filename), filename, sizeof(header.filename));
	stat(filename, &(header.fileinfo));

	struct sockaddr_in server_addr;

	char server_name[50];
	ping(&header, &server_addr, server_name, sizeof(server_name));
	printf("Got response from %s\n", server_name);
	printf("Waiting for server to accept file...\n");

	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	check(sockfd, "socket");
	
	check(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)), "connect");
	
	int confirm = 0;
	int recvd = recv(sockfd, &confirm, sizeof(confirm), 0);
	if (recvd < 1 || confirm < 1)
	{
		printf("%s refused file\n", server_name);
		close(sockfd);
		exit(0);
	}

	printf("Connected to %s\n", server_name);


	FILE *file = fopen(filename, "rb");
	unsigned int buffsize = header.fileinfo.st_blksize;
	void *buffer = (void *)(malloc(buffsize));

	int n;
	long int total_sent= 0;
	while (n = fread(buffer, 1, buffsize, file))
	{
		total_sent += n;
		update_bar(100*total_sent/header.fileinfo.st_size, 50);
		send(sockfd, buffer, n, 0);
	}
	printf("\nSent %s to %s\n", header.filename, server_name);
}

void check(int n, char *str){
	if (n == -1)
	{
		perror(str);
		exit(1);
	}
}

void zero_out(struct sockaddr_in *ad){
    memset(&(ad->sin_zero), 0, sizeof(ad->sin_zero));
}

void ping(struct ping_header *header, struct sockaddr_in *server_addr, char *server_name, int size)
{
	int sockfd;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	check(sockfd, "socket");

	// change socket option to allow broadcast
	int b = 1;
	check(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &b, sizeof(b)), "setsockopt");

	// broadcast address
	struct sockaddr_in broadcast;
	broadcast.sin_family = AF_INET;
	broadcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	broadcast.sin_port = htons(PORT);
	zero_out(&broadcast);


	// send a  ping to all devices so  server can find out address
	check(sendto(sockfd, header, sizeof(struct ping_header), MSG_CONFIRM, (struct sockaddr *)&broadcast, sizeof(broadcast)), "send");
	// puts("sent ping");
	close(sockfd);

	// socket to listen to response
	int listen_socket;
	listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	check(listen_socket, "socket");
	check(set_time_out(listen_socket, TIMEOUT_USEC), "set_time_out");

	// host addr
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(PORT);
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind to host
	check(bind(sockfd, (const struct sockaddr *)&myaddr, sizeof(myaddr)), "bind");
	socklen_t t = sizeof(struct sockaddr);
	check(recvfrom(sockfd, server_name, size, MSG_WAITALL, (struct sockaddr *)server_addr, &t), "recv");

	close(listen_socket);
}

int set_time_out(int sockfd, long int usecs)
{
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = TIMEOUT_USEC;

	return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}
