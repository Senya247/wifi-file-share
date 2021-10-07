#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include "progress_bar.h"

#define KB 1000
#define MB 1000000
#define GB 1000000000
#define TB 1000000000000

#define PORT 8888

struct ping_header
{
	char name[50];
	char filename[50];
	struct stat fileinfo;
};

void check(int n, char *str);
void zero_out(struct sockaddr_in *ad);
void recv_ping(struct ping_header *header, struct sockaddr_in *client_addr);
void filesize(long int size, char *buffer, int len);

int main(int argc, char *argv[])
{
	// header to be received
	struct ping_header header;
	struct sockaddr_in client_addr;

	recv_ping(&header, &client_addr);
	char *client_name = header.name;
	zero_out(&client_addr);

	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	check(sockfd, "socket");

	// setting socket options to allow reusing
	int option = 1;
	check(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)), "setsockopt");

	// host address
	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(PORT);

	check(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in)), "bind");

	listen(sockfd, 2);

	// No need to pass client addr or addrlen, already determined before
	socklen_t t = sizeof(client_addr);
	int newsock = accept(sockfd, (struct sockaddr *)&client_addr, &t);

	char fsize[10];
	filesize(header.fileinfo.st_size, fsize, sizeof(fsize));
	printf("Receive %s of size %s from %s?(y/n)?: ", header.filename, fsize, client_name);
	int confirm = fgetc(stdin) == 'y' ? 1 : 0;

	int sent = send(newsock, &confirm, sizeof(confirm), 0);
	if (confirm < 1){
		close(sockfd);
		close(newsock);
		exit(0);
	}
	else if (sent < 1){
		puts("Could not send signal to receive file");
		close(sockfd);
		close(newsock);
	}

	printf("Connected to %s\n", client_name);


	FILE *outfile = fopen(header.filename, "wb");
	unsigned int buffsize = header.fileinfo.st_blksize;
	
	void *buffer = (void *)(malloc(buffsize));
	int received;
	long int total_recvd = 0;

	while (received = read(newsock, buffer, buffsize))
	{
		total_recvd += received;
		update_bar(100 * total_recvd/header.fileinfo.st_size, 50);
		fwrite(buffer, 1, received, outfile);
	}
	printf("\nReceived %s from %s\n", header.filename, client_name);
}

void check(int n, char *str)
{
	if (n == -1)
	{
		perror(str);
		exit(1);
	}
}

void zero_out(struct sockaddr_in *ad)
{
	memset(&(ad->sin_zero), 0, sizeof(ad->sin_zero));
}

void recv_ping(struct ping_header *header, struct sockaddr_in *client_addr)
{
	int sockfd;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	check(sockfd, "socket");

	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY; // puts device address
	zero_out(&my_addr);

	check(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)), "bind");

	// receiving headers
	socklen_t n = sizeof(&client_addr);
	puts("Waiting for connections...");
	check(recvfrom(sockfd, header, sizeof(struct ping_header), MSG_WAITALL, (struct sockaddr *)client_addr, &n), "recvfrom");
	zero_out(client_addr);

	// For some reason, the port number changes when client_addr is filled
	client_addr->sin_port = htons(PORT);

	// header to hold this devic's name
	struct ping_header out_header;
	gethostname(out_header.name, sizeof(out_header.name));

	// for some reason, putting sizeof(&client_addr) into size parameter doesnt work
	check(sendto(sockfd, (void *)&out_header, sizeof(struct ping_header), MSG_CONFIRM, (struct sockaddr *)client_addr, sizeof(*client_addr)), "sendto");
	close(sockfd);
}

void filesize(long int size, char *buffer, int len)
{
	if (size < KB)
    {
		snprintf(buffer, len, "%ld bytes", size);
    }
    else if ((size > KB) && (size < MB))
    {
		snprintf(buffer, len, "%.2f kb", (float)size/KB);
    }
    else if ((size > MB) && (size < GB))
    {
		snprintf(buffer, len, "%.2f mb", (float)size/MB);
    }
    else if  ((size > GB) && (size < TB))
    {
		snprintf(buffer, len, "%.2f gb", (float)size/GB);
    }
}
