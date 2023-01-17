/* 
* COEN233 Fall 2021 assignment 1 server
* run ./server <portno>
* Author: Heng-Yu Wu 1608635
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

const int num_packets = 5;
// Design a protocal with the following primitives
const short START_ID = 0xFFFF;
const short END_ID = 0xFFFF;
const char LENGTH = 0xFF;

// Packet types
const short DATA = 0xFFF1;
const short ACK = 0xFFF2;
const short REJECT = 0xFFF3;

// Reject sub codes
const short REJECT_OOS = 0xFFF4; // out of sequence
const short REJECT_LM = 0xFFF5; // length mismatch
const short REJECT_EOPM = 0xFFF6; // end of packet missing
const short REJECT_DP = 0xFFF7; // duplicate packet

typedef struct data_packet {
	short start_id;
	char client_id;
	short data;
	char seg_num;
	unsigned char length;
	char payload[255];
	short end_id;
} data_packet;

typedef struct ack_packet {
	short start_id;
	char client_id;
	short ack;
	char seg_num_recv;
	short end_id;
} ack_packet;

typedef struct rej_packet {
	short start_id;
	char client_id;
	short rej;
	short rej_sub;
	char rej_seg_num;
	short end_id;
} rej_packet;

void print_error(char* error_message){
    printf("\033[1;31m");
    printf("%s", error_message);
    printf("\033[0m");
}

int main(int argc, char *argv[]) {
	int sock, portno;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	data_packet data_p;
	rej_packet rej_p;
	ack_packet ack_p;
	
	if (argc < 2) {
		fprintf(stderr, "Error! No port provided.\n");
		exit(1);
	}
	
	// create a udp socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	
	bzero((char *) &server_addr, sizeof(server_addr));
	portno = atoi(argv[1]);
	
	// "the sin_family member of struct sockaddr_in must always be AF_INET"
    // ref: https://stackoverflow.com/questions/57779761/why-does-the-sin-family-member-exist
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portno);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	
	
	// Bind socket descriptor to the server address
	bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
	
	printf("\nServer: Listening on port %d...\n\n", portno);
	
	// receive data from client
	int expected_seg = 0;
	while (1) {
		expected_seg = expected_seg % num_packets;
		recvfrom(sock, &data_p, sizeof(data_packet), 0, (struct sockaddr*) &client_addr, &addrlen);
		
		printf("\nServer: Message received: ");
		printf("%s length: %d, seg_num: %d\n", data_p.payload, data_p.length, data_p.seg_num);
		
		rej_p.start_id = START_ID;
		rej_p.client_id = data_p.client_id;
		rej_p.rej = REJECT;
		rej_p.rej_seg_num = data_p.seg_num;
		rej_p.end_id = END_ID;

		if (data_p.seg_num < expected_seg) {
			rej_p.rej_sub = REJECT_DP;
			print_error("Server: Error! Duplicate packet.\n");
		} else if (data_p.length != sizeof(data_p.payload)) {
			rej_p.rej_sub = REJECT_LM;
			print_error("Server: Error! Length mismatch.\n");
		} else if (data_p.end_id != END_ID) {
			rej_p.rej_sub = REJECT_EOPM;
			print_error("Server: Error! End of packet missing.\n");
		} else if (data_p.seg_num != expected_seg) {
			rej_p.rej_sub = REJECT_OOS;
			print_error("Server: Error! Out of sequence.\n");
		} else {
			// construct ack message
			ack_p.start_id = START_ID;
			ack_p.client_id = data_p.client_id;
			ack_p.ack = ACK;
			ack_p.seg_num_recv = data_p.seg_num;
			ack_p.end_id = END_ID;
			
			// send ack
			printf("Server: Sending ACK...\n");
			sendto(sock, &ack_p, sizeof(ack_packet), 0, (struct sockaddr *) &client_addr, addrlen);
			expected_seg ++;
			continue;
		}
		
		// send reject message
		sendto(sock, &rej_p, sizeof(rej_packet), 0, (struct sockaddr *) &client_addr, addrlen);
	}
	
}