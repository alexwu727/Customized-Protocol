/* 
* COEN233 Fall 2021 assignment 1 client
* run ./client <hostname> <portno>
* Author: Heng-Yu Wu 1608635
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <poll.h>

const int num_packets = 5;
// Design a protocal with the following primitives
const short START_ID = 0xFFFF;
const short END_ID = 0xFFFF;
const char CLIENT_ID = 0xFF;
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

// Data packet format
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
	char seg_num_recv;
	short end_id;
} rej_packet;

typedef struct res_packet {
	short start_id;
	char client_id;
	short type;
	char unknown_content[5];
} res_packet;


void send_messages(data_packet packets[num_packets], data_packet backup_packets[num_packets], int sock, char* hostname, int portno, struct sockaddr_in server_addr, socklen_t addrlen) {
	printf("\n================================================================\n");
	
	int n;
	struct sockaddr_in client_addr;
	data_packet data_p;
	ack_packet ack_p;
	rej_packet rej_p;
	res_packet res_p;
	
	// using poll as an act timer
	struct pollfd pfdsock;
	pfdsock.fd = sock;
	pfdsock.events = POLLIN;
	
	int curr_seg = 0;
	int send_error_packet = 1;
	
	while (1) {
		// while sending error_data_packets, send error packet first. After that, resend the correct packet.
		if (send_error_packet) {
			data_p = packets[curr_seg];
		} else {
			data_p = backup_packets[curr_seg];
		}
		
		// send packets
		int attempt = 1;
		
		while (1) {
			if (attempt > 3) { 
                printf("\033[1;31m");
				printf("\nServer does not respond.\n");
                printf("\033[0m");
				exit(1);
			}

			printf("Sending packet no. %d to %s port %d... (attempt %d of 3)\n", data_p.seg_num, hostname, portno, attempt);
				
			sendto(sock, &data_p, sizeof(data_packet), 0, (struct sockaddr *) &server_addr, addrlen);
			
			n = poll(&pfdsock,1,3000);      // wait for 3 sec
			
			if (n == 0) {                   // process times out
                printf("\033[1;31m");
                printf("Server no response.\n");
                printf("\033[0m");
                attempt++;
				continue;
            }
			
			recvfrom(sock, &res_p, sizeof(res_packet), 0, (struct sockaddr *) &client_addr, &addrlen);
			break;
		}
		
        // read the message from server
		if (res_p.type == ACK) {
			ack_p = *((ack_packet *) &res_p);
			printf("Got an ack for packet no. %d.\n\n", ack_p.seg_num_recv);
			if (curr_seg == num_packets - 1) { // finish
				break;
			} else {
				curr_seg ++;
				send_error_packet = 1;
			}
		} else if (res_p.type == REJECT) {
			rej_p = *((rej_packet *) &res_p);
			if (rej_p.rej_sub == REJECT_OOS) {
                printf("\033[1;31m");
				printf("Server: Error! Out of sequence (seg no.%d)\n\n", rej_p.seg_num_recv);
                printf("\033[0m");
			} else if (rej_p.rej_sub == REJECT_LM) {
                printf("\033[1;31m");
				printf("Server: Error! Length mismatch (seg no.%d)\n\n", rej_p.seg_num_recv);
                printf("\033[0m");
			} else if (rej_p.rej_sub == REJECT_EOPM) {
                printf("\033[1;31m");
				printf("Server: Error! End of packet missing (seg no.%d)\n\n", rej_p.seg_num_recv);
                printf("\033[0m");
			} else if (rej_p.rej_sub == REJECT_DP) {
                printf("\033[1;31m");
				printf("Server: Error! Duplicate packet (seg no.%d)\n\n", rej_p.seg_num_recv);
                printf("\033[0m");
			} else {
                printf("\033[1;31m");
				printf("\nUnknown error type from server (seg no.%d)\n\n", rej_p.seg_num_recv);
                printf("\033[0m");
			}
			
			// If rejected by server, backup packet will be sent in next message.
			send_error_packet = 0;
		} else {
            printf("\033[1;31m");
			printf("Unknown response from server \n");
            printf("\033[0m");
		}
		
	}

}

int main(int argc, char *argv[]) {
	
	int sock, portno;
    // The SOCKADDR_IN structure specifies a transport address and port for the AF_INET address family.
	struct sockaddr_in server_addr;
//	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct hostent *hp;
	char buffer[255];
	
	if (argc < 3) {
		printf("\033[1;31m");
		printf("Error! No port provided.\n");
		printf("\033[0m");
		exit(1);
	}
	
	// create a udp socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) { 
		printf("\033[1;31m");
		printf("Error while creating socket.\n"); 
		printf("\033[0m");
		}
	
	hp = gethostbyname(argv[1]);
	
	portno = atoi(argv[2]);
	
	bcopy((char *)hp->h_addr, (char *)&server_addr.sin_addr, hp->h_length);
    // "the sin_family member of struct sockaddr_in must always be AF_INET"
    // ref: https://stackoverflow.com/questions/57779761/why-does-the-sin-family-member-exist
	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	
	
	
	// construct correct packets
	data_packet correct_data_packets[num_packets];
	for (int i = 0; i < num_packets; i ++) {
		correct_data_packets[i].start_id = START_ID;
		correct_data_packets[i].client_id = CLIENT_ID;
		correct_data_packets[i].data = DATA;
		correct_data_packets[i].seg_num = i;
		bzero(buffer, 255);
		sprintf(buffer, "This is packet no. %d from client.", i);
		strcpy(correct_data_packets[i].payload, buffer);
		correct_data_packets[i].length = LENGTH;
		correct_data_packets[i].end_id = END_ID;
	}
	
	// construct error packets
	data_packet error_data_packets[num_packets];
	for (int i = 0; i < num_packets; i ++) {
		error_data_packets[i].start_id = START_ID;
		error_data_packets[i].client_id = CLIENT_ID;
		error_data_packets[i].data = DATA;
		error_data_packets[i].seg_num = i;
		bzero(buffer, 255);
		sprintf(buffer, "This is packet no. %d from client.", i);
		strcpy(error_data_packets[i].payload, buffer);
		error_data_packets[i].length = LENGTH;
		error_data_packets[i].end_id = END_ID;
	}
	
	error_data_packets[1].seg_num = 2; // error case-1
	error_data_packets[2].length = 0x0C; // error case-2
	error_data_packets[3].end_id = 0xFFF0; // error case-3
	error_data_packets[4].seg_num = 3; // error case-4
	
	// sending correct packets
	send_messages(correct_data_packets, correct_data_packets, sock, argv[1], portno, server_addr, addrlen);
	
	// sending error packets
	send_messages(error_data_packets, correct_data_packets, sock, argv[1], portno, server_addr, addrlen);
	
	
}