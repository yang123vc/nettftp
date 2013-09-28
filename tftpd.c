// Simple TFTP server implementation.
// Author: Sol Boucher <slb1566@rit.edu>

#include "tftp_protoc.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *connection(void *);
static void strtolower(char *, size_t);

int main(void)
{
	int socketfd = openudp(PORT);

	while(1)
	{
		struct sockaddr_in saddr_remote;
		socklen_t sktaddrmt_len = sizeof saddr_remote;
		void *request = NULL;
		ssize_t req_len = 0; // Includes extra terminator

		request = recvpkta(socketfd, &req_len, &saddr_remote, &sktaddrmt_len);

		uint16_t opcode = *(uint16_t *)request;
		char *filename = (char *)(request+2);
		size_t fname_len = strlen(filename);
		char *mode = (char *)(filename+fname_len+1);
		size_t mode_len = strlen(mode);
		strtolower(mode, mode_len);

		if(opcode == OPC_RRQ)
			printf("opcode: RRQ\n");
		else if(opcode == OPC_WRQ)
			printf("opcode: WRQ\n");
		else
			printf("unexpected opcode!\n");
		printf("filename: %s\n", filename);
		printf("xfermode: %s\n", mode);
		putchar('\n');

		if((opcode == OPC_RRQ || opcode == OPC_WRQ) && req_len == 2+fname_len+1+mode_len+1)
		{
			pthread_t thread;
			void *actuals = malloc(sktaddrmt_len+2+fname_len+1);
			*(struct sockaddr_in *)actuals = saddr_remote;
			*(uint16_t *)(actuals+sktaddrmt_len) = opcode;
			memcpy(actuals+sktaddrmt_len+2, filename, fname_len+1);
			pthread_create(&thread, NULL, &connection, actuals);
		}
		else
			senderr(socketfd, ERR_ILLEGALOPER, &saddr_remote);

		if(request)
			free(request);
	}

	return 0;
}

void *connection(void *args)
{
	printf("spawned a thread!\n");

	struct sockaddr_in *rmtsocket = (struct sockaddr_in *)args;
	socklen_t rmtskt_len = sizeof(struct sockaddr_in);
	uint16_t oper = *(uint16_t *)(args+rmtskt_len);
	char *filename = (char *)(args+rmtskt_len+2);
	printf("oper: %hu\nname: %s\n", oper, filename);
	int locsocket = openudp(0);

	struct sockaddr_in mysock;
	socklen_t mysck_len = sizeof mysock;
	getsockname(locsocket, (struct sockaddr *)&mysock, &mysck_len);
	printf("server: %hu\n", ntohs(mysock.sin_port)); // TODO Remove all this if unused
	printf("client: %hu\n", ntohs(rmtsocket->sin_port));
	putchar('\n');

	int fd;
	if((fd = open(filename, oper == OPC_WRQ ? O_WRONLY|O_CREAT|O_EXCL : O_RDONLY, 0666)) < 0)
	{
		diagerrno(locsocket, rmtsocket);
		free(args);
		return NULL;
	}

	if(oper == OPC_RRQ)
		sendfile(locsocket, fd, rmtsocket);
	else // oper == OPC_WRQ
	{
		uint16_t ack[2];
		ack[0] = OPC_ACK;
		ack[1] = (uint16_t)0;
		sendto(locsocket, ack, sizeof ack, 0, (struct sockaddr *)rmtsocket, rmtskt_len);

		recvfile(locsocket, fd);
	}

	close(fd);
	free(args);
	return NULL;
}

void strtolower(char *a, size_t l)
{
	int index;
	for(index = 0; index < l; ++index)
		a[index] = tolower(a[index]);
}
