#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rtp.h"

/* GIVEN Function:
 * Handles creating the client's socket and determining the correct
 * information to communicate to the remote server
 */
CONN_INFO* setup_socket(char* ip, char* port){
	struct addrinfo *connections, *conn = NULL;
	struct addrinfo info;
	memset(&info, 0, sizeof(struct addrinfo));
	int sock = 0;

	info.ai_family = AF_INET;
	info.ai_socktype = SOCK_DGRAM;
	info.ai_protocol = IPPROTO_UDP;
	getaddrinfo(ip, port, &info, &connections);

	/*for loop to determine corr addr info*/
	for(conn = connections; conn != NULL; conn = conn->ai_next){
		sock = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
		if(sock <0){
			if(DEBUG)
				perror("Failed to create socket\n");
			continue;
		}
		if(DEBUG)
			printf("Created a socket to use.\n");
		break;
	}
	if(conn == NULL){
		perror("Failed to find and bind a socket\n");
		return NULL;
	}
	CONN_INFO* conn_info = malloc(sizeof(CONN_INFO));
	conn_info->socket = sock;
	conn_info->remote_addr = conn->ai_addr;
	conn_info->addrlen = conn->ai_addrlen;
	return conn_info;
}

void shutdown_socket(CONN_INFO *connection){
	if(connection)
		close(connection->socket);
}

/* 
 * ===========================================================================
 *
 *			STUDENT CODE STARTS HERE. PLEASE COMPLETE ALL FIXMES
 *
 * ===========================================================================
 */


/*
 *  Returns a number computed based on the data in the buffer.
 */
static int checksum(char *buffer, int length){

	/*  ----  FIXME  ----
	 *
	 *  The goal is to return a number that is determined by the contents
	 *  of the buffer passed in as a parameter.  There a multitude of ways
	 *  to implement this function.  For simplicity, simply sum the ascii
	 *  values of all the characters in the buffer, and return the total.
	 */ 
	 int sum = 0;
	 int i;
	 for (i = 0; i < length; i++) {
	 	sum += (int)buffer[i];
	 }
	 return sum;
}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it 
 *  contains the length of the array created.
 */
static PACKET* packetize(char *buffer, int length, int *count){

	/*  ----  FIXME  ----
	 *  The goal is to turn the buffer into an array of packets.
	 *  You should allocate the space for an array of packets and
	 *  return a pointer to the first element in that array.  Each 
	 *  packet's type should be set to DATA except the last, as it 
	 *  should be LAST_DATA type. The integer pointed to by 'count' 
	 *  should be updated to indicate the number of packets in the 
	 *  array.
	 */ 

	 /*
	 * we use the last byte of each packet as the null terminate
	 * so the actual max payload length is MAX_PAYLOAD_LENGTH - 1
	 */
	 int max_payload = MAX_PAYLOAD_LENGTH - 1;
	 int size = length / max_payload;
	 if (length % max_payload != 0) {
	 	size++;
	 }

	 PACKET* parr;
	 parr = (PACKET*)malloc(sizeof(PACKET) * size);
	 int i, j;
	 for (i = 0; i < size; i++) {
	 	if (i != size - 1) {
	 		parr[i].type = DATA;
	 		parr[i].payload_length = max_payload;
	 	} else {
	 		parr[i].type = LAST_DATA;
	 		if (length % max_payload != 0) {
	 			parr[i].payload_length = length % max_payload;
	 		} else {
	 			parr[i].payload_length = max_payload;
	 		}
	 	}
	 	
	 	j = 0;
	 	int sum = 0;
	 	while (j < max_payload && i * max_payload + j < length) {
	 		(parr[i].payload)[j] = buffer[i * max_payload + j];
	 		sum += buffer[i * max_payload + j];
	 		j++;
	 	}
	 	(parr[i].payload)[j] = '\0';
	 	parr[i].checksum = sum;
	 }

	 *count = size;
	 return parr;
}

/*
 * Send a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
int rtp_send_message(CONN_INFO *connection, MESSAGE*msg){
	/* ---- FIXME ----
	 * The goal of this function is to turn the message buffer
	 * into packets and then, using stop-n-wait RTP protocol,
	 * send the packets and re-send if the response is a NACK.
	 * If the response is an ACK, then you may send the next one
	 */
	 int* count = (int*)malloc(sizeof(int));
	 *count = 0;
	 PACKET* packet = packetize(msg -> buffer, msg -> length, count);
	 int size = *count;

	 int socketfd = connection -> socket;
	 struct sockaddr* src_addr = connection -> remote_addr;
	 socklen_t addrlen = connection -> addrlen;
	 socklen_t* paddrlen = &(addrlen);

	 PACKET* recv = (PACKET*)malloc(sizeof(PACKET));
	 while (1) {
	 	sendto(socketfd, packet, sizeof(PACKET), 0, src_addr, addrlen);
	 	recvfrom(socketfd, recv, sizeof(PACKET), 0, src_addr, paddrlen);
	 	if (recv -> type == ACK) {
	 		packet = packet + 1;
	 		size--;
	 		if (size == 0) {
	 			break;
	 		}
	 	} else if (recv -> type != ACK && recv -> type != NACK) {
	 		return -1;
	 	}
	 }

	 return 0;
}

/*
 * Receive a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
MESSAGE* rtp_receive_message(CONN_INFO *connection){
	/* ---- FIXME ----
	 * The goal of this function is to handle 
	 * receiving a message from the remote server using
	 * recvfrom and the connection info given. You must 
	 * dynamically resize a buffer as you receive a packet
	 * and only add it to the message if the data is considered
	 * valid. The function should return the full message, so it
	 * must continue receiving packets and sending response 
	 * ACK/NACK packets until a LAST_DATA packet is successfully 
	 * received.
	 */

	 MESSAGE* message = (MESSAGE*)malloc(sizeof(MESSAGE));
	 message -> length = 0;
	 PACKET* packet = (PACKET*)malloc(sizeof(PACKET));
	  /*
	 * we use the last byte of each message as the null terminate
	 * so first allocate this byte
	 */
	 char* message_buffer = (char*)malloc(sizeof(char));

	 int socketfd = connection -> socket;
	 struct sockaddr* src_addr = connection -> remote_addr;
	 socklen_t addrlen = connection -> addrlen;
	 socklen_t* paddrlen = &(addrlen);
	 int current_char_pos = 0;

	 while (1) {
	 	recvfrom(socketfd, packet, sizeof(PACKET), 0, src_addr, paddrlen);
		
		if (checksum(packet -> payload, packet -> payload_length) == packet -> checksum) {
			message_buffer = (char*)realloc(message_buffer, ((packet -> payload_length) + (message -> length)) * sizeof(char));
			int j = 0;
			for (j = 0; j < packet -> payload_length; j++) {
				message_buffer[current_char_pos] = (packet -> payload)[j];
				current_char_pos++;
			}

			message -> length = message -> length + packet -> payload_length;
			PACKET* ackpacket = (PACKET*)malloc(sizeof(PACKET));
			ackpacket -> type = ACK;
			sendto(socketfd, ackpacket, sizeof(PACKET), 0, src_addr, addrlen);
			if (packet -> type == LAST_DATA) {
				message_buffer[current_char_pos] = '\0'; // add null terminate
				break;
			}
		} else {
			PACKET* nackpacket = (PACKET*)malloc(sizeof(PACKET));
			nackpacket -> type = NACK;
			sendto(socketfd, nackpacket, sizeof(PACKET), 0, src_addr, addrlen);
		}
	 	
	 }
	 message -> buffer = message_buffer;
	 return message;

}
