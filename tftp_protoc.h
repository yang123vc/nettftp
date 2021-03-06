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

// Simple TFTP protocol library.
// Author: Sol Boucher <slb1566@rit.edu>

#ifndef TFTP_PROTOC_H
#define TFTP_PROTOC_H

#include <netinet/in.h>

// Connection parameters:
const in_port_t PORT_PRIVILEGED;
const in_port_t PORT_UNPRIVILEGED;
const size_t DATA_LEN;

// Protocol details:
const uint16_t OPC_RRQ;
const uint16_t OPC_WRQ;
const uint16_t OPC_DAT;
const uint16_t OPC_ACK;
const uint16_t OPC_ERR;
const char *const MODE_ASCII;
const char *const MODE_OCTET;
const uint16_t ERR_UNKNOWN;
const uint16_t ERR_NOTFOUND;
const uint16_t ERR_ACCESSDENIED;
const uint16_t ERR_DISKFULL;
const uint16_t ERR_ILLEGALOPER;
const uint16_t ERR_UNKNOWNTID;
const uint16_t ERR_CLOBBER;
const uint16_t ERR_UNKNOWNUSER;

typedef int bool;

// Utility functions:
int openudp(uint16_t);
void *recvpkt(int);
void *recvpkta(int, struct sockaddr_in *);
void *recvpktal(int, size_t *, struct sockaddr_in *);
void sendfile(int, int, struct sockaddr_in *);
const char *recvfile(int, int);
void sendack(int, uint16_t, struct sockaddr_in *);
void diagerrno(int, struct sockaddr_in *);
void senderr(int, uint16_t, struct sockaddr_in *);
bool iserr(void *);
const char *strerr(void *);
void handle_error(const char *);

#endif
