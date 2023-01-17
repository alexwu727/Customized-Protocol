/* 
* COEN233 Fall 2021 assignment 2 client
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

// client will send 4 requests to server
const int num_requests = 4; 

const short START_ID = 0xFFFF;
const short END_ID = 0xFFFF;
const char CLIENT_ID = 0xFF;
const char MAX_LENGTH = 0xFF;
const unsigned long MAX_SUB_NUM = 0xFFFFFFFF;

const short ACC_PER = 0xFFF8; // Access permission request packet

const short NOT_PAID = 0xFFF9; // Subscriber has not paid
const short NOT_EXIST = 0xFFFA; // Subscriber does not exist on database
const short ACC_OK = 0xFFFB; // Subscriber permitted to access the network

// Technology
const char TECH_2G = 0x02;
const char TECH_3G = 0x03;
const char TECH_4G = 0x04;
const char TECH_5G = 0x05;

typedef struct data_packet {
	short start_id;
	char client_id;
	short type;
	char seg_num;
	unsigned char length;
	char tech;
	unsigned long subscriber_num;
	short end_id;
} data_packet;

void print_error(char* error_message){
    printf("\033[1;31m");
    printf("%s", error_message);
    printf("\033[0m");
}


void send_messages(data_packet packets[num_requests], int sock, char* hostname, int portno, struct sockaddr_in server_addr, socklen_t addrlen) {
	int n;
	struct sockaddr_in client_addr;
	data_packet request_p;
	data_packet respond_p;
	
	// using poll as an act timer
	struct pollfd pfdsock;
	pfdsock.fd = sock;
	pfdsock.events = POLLIN;
	
	int curr_req = 0;
	
	while (1) {
		
		request_p = packets[curr_req];
		
		// send packets
		int attempt = 1;
		
		while (1) {
			if (attempt > 3) { 
				print_error("\nServer does not respond.\n");
				exit(1);
			}
			printf("Subscriber %lu with %dG\n", request_p.subscriber_num, request_p.tech);
			printf("Sending access request to %s port %d (attempt %d of 3)\n", hostname, portno, attempt);
						
			sendto(sock, &request_p, sizeof(data_packet), 0, (struct sockaddr *) &server_addr, addrlen);
			
			n = poll(&pfdsock,1,3000);
			
			if (n == 0) {                   // process times out
                print_error("Server no response.\n");
                attempt++;
				continue;
            }
			
			recvfrom(sock, &respond_p, sizeof(data_packet), 0, (struct sockaddr *) &client_addr, &addrlen);
			
			break;
		}
		
		// read the message from server
		if (respond_p.type == ACC_OK) {
			printf("Server: Access permitted.\n\n");
		} else if (respond_p.type == NOT_PAID) {
			print_error("Server: Denied, subscriber has not paid.\n\n");
		} else if (respond_p.type == NOT_EXIST) {
			print_error("Server: Denied, subscriber does not exist on database.\n\n");
		} else {
			print_error("Unknown response from server.\n\n");
		}
		
		if (curr_req == num_requests - 1) {		// finish
			break;
		} else {
			curr_req ++;
		}
		
	}
	
}

int main(int argc, char *argv[]) {
	
	int sock, portno;
	struct sockaddr_in server_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	struct hostent *hp;
	
	if (argc < 3) {
		print_error("Error! No port provided.\n");
		exit(1);
	}
	
	// create a udp socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) { 
		print_error("Error while creating socket.\n"); 
		}
	
	server_addr.sin_family = AF_INET;
	
	hp = gethostbyname(argv[1]);
	
	portno = atoi(argv[2]);
	
	bcopy((char *)hp->h_addr, (char *)&server_addr.sin_addr, hp->h_length);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(portno);
	
	// construct requests to send
	unsigned long sub_nums[num_requests];
	char technologies[num_requests];
	
	sub_nums[0] = 0000000000;	// wrong num
	sub_nums[1] = 4085546805;	// wrong tech
	sub_nums[2] = 4086668821;	// not paid
	sub_nums[3] = 4086808821;
	
	technologies[0] = 05;
	technologies[1] = 05;	
	technologies[2] = 03;
	technologies[3] = 02;
	
	data_packet request_array[num_requests];
	for (int i = 0; i < num_requests; i ++) {
		request_array[i].start_id = START_ID;
		request_array[i].client_id = CLIENT_ID;
		request_array[i].type = ACC_PER;
		request_array[i].seg_num = i;
		request_array[i].length = MAX_LENGTH;
		request_array[i].tech = technologies[i];
		request_array[i].subscriber_num = sub_nums[i];
		request_array[i].end_id = END_ID;
	}
	
	// sending requests
	send_messages(request_array, sock, argv[1], portno, server_addr, addrlen);
	
}