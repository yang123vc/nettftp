/*
 * Copyright (C) 2013 Sol Boucher
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with it.  If not, see <http://www.gnu.org/licenses/>.
 */

// Simple TFTP server implementation.
// Author: Sol Boucher <slb1566@rit.edu>

#include "tftp_protoc.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *connection(void *);
static void strtolower(char *);

// Runs the main loop that accepts read and write requests.
// Returns: exit status
int main(void)
{
	// Bind to a privileged port if possible, but fall back if necessary:
	int socketfd = openudp(PORT_PRIVILEGED);
	if(socketfd < 0)
	{
		socketfd = openudp(PORT_UNPRIVILEGED);
		if(socketfd < 0)
			handle_error("bind()");
	}

	// As requests are received, spawn threads or return error datagrams:
	struct sockaddr_in saddr_remote;
	while(1)
	{
		// Receive each incoming request:
		void *request = recvpkta(socketfd, &saddr_remote);

		// Make sure it has a request opcode:
		uint16_t opcode = *(uint16_t *)request;
		if(opcode == OPC_RRQ || opcode == OPC_WRQ)
		{
			// Split it up into usable chunks:
			const char *filename = (char *)(request+2);
			size_t fname_len = strlen(filename);
			char *mode = (char *)(filename+fname_len+1);
			strtolower(mode);

#ifdef DEBUG
			fprintf(stderr, "received a request:\n");
			if(opcode == OPC_RRQ)
				fprintf(stderr, "opcode: RRQ\n");
			else if(opcode == OPC_WRQ)
				fprintf(stderr, "opcode: WRQ\n");
			else
				fprintf(stderr, "unexpected opcode!\n");
			fprintf(stderr, "filename: %s\n", filename);
			fprintf(stderr, "xfermode: %s\n", mode);
			fprintf(stderr, "\n");
#endif

			// Pass all relevant data off to a separate thread:
			pthread_t thread;
			void *actuals = malloc(sizeof(saddr_remote)+2+fname_len+1);
			*(struct sockaddr_in *)actuals = saddr_remote;
			*(uint16_t *)(actuals+sizeof saddr_remote) = opcode;
			memcpy(actuals+sizeof(saddr_remote)+2, filename, fname_len+1);
			pthread_create(&thread, NULL, &connection, actuals);
		}
		else
			senderr(socketfd, ERR_ILLEGALOPER, &saddr_remote);

		if(request)
			free(request);
	}

	return 0;
}

// Manages each file transfer requested of the server.
// Accepts: sockaddr_in of client, unsigned 16-bit opcode, null-terminated filename
// Returns: NULL
void *connection(void *args)
{
	// Let's be reasonable and stop dealing with The Blob:
	struct sockaddr_in *rmtsocket = (struct sockaddr_in *)args;
	socklen_t rmtskt_len = sizeof(struct sockaddr_in);
	uint16_t oper = *(uint16_t *)(args+rmtskt_len);
	char *filename = (char *)(args+rmtskt_len+2);

	// Open up an ephemeral port for the transfer:
	int locsocket = openudp(0);
	if(locsocket < 0)
		handle_error("bind()");

#ifdef DEBUG
	fprintf(stderr, "spawned a thread!\n");
	fprintf(stderr, "oper: %hu\nname: %s\n", oper, filename);
	struct sockaddr_in mysock;
	socklen_t mysck_len = sizeof mysock;
	getsockname(locsocket, (struct sockaddr *)&mysock, &mysck_len);
	fprintf(stderr, "server: %hu\n", ntohs(mysock.sin_port));
	fprintf(stderr, "client: %hu\n", ntohs(rmtsocket->sin_port));
	fprintf(stderr, "\n");
#endif

	// Try to open the file for reading *or* writing, as appropriate:
	int fd;
	if((fd = open(filename, oper == OPC_WRQ ? O_WRONLY|O_CREAT|O_EXCL : O_RDONLY, 0666)) < 0)
	{
		diagerrno(locsocket, rmtsocket);
		free(args);
		return NULL;
	}

	// Send or receive file contents, as appropriate:
	if(oper == OPC_RRQ)
		sendfile(locsocket, fd, rmtsocket);
	else // oper == OPC_WRQ
	{
		sendack(locsocket, 0, rmtsocket);
		recvfile(locsocket, fd);
	}

	close(fd);
	free(args);
	return NULL;
}

// Converts a string to lowercase in-place.
// Accepts: null-terminated string
void strtolower(char *a)
{
	int index;
	for(index = 0; a[index]; ++index)
		a[index] = tolower(a[index]);
}
