/* 
* COEN233 Fall 2021 assignment 2 server
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

const int DB_size = 10;

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
	char cli_id;
	short type;
	char seg_num;
	unsigned char length;
	char tech;
	unsigned long subscriber_num;
	short end_id;
} data_packet;

typedef struct account {
	unsigned long subscriber_num;
	char tech;
	char is_paid;
} account;

void print_error(char* error_message){
    printf("\033[1;31m");
    printf("%s", error_message);
    printf("\033[0m");
}

/*
* Given a subscriber number and technology, search for matching account in the data base.
* -> 0 if subscriber does not exist
* -> 1 if subscriber has not paid 
* -> 2 if subscriber permitted to access the network 
*/
char checkDB(unsigned long number, char tech, account accounts[DB_size], int num_accouts) {
	for (int i = 0; i < num_accouts; i++) {
		if (accounts[i].subscriber_num == number) {
			if (!accounts[i].is_paid) {return 1; } 
			if (accounts[i].tech != tech - 0) { return 0;}
			return 2;
			}
		}
	return 0;
}
	


// read database into an array. return number of entries.
int loadDB(char fname[50], account accounts[DB_size]) {
	FILE *fptr;
	unsigned long number;
	int tech, is_paid;
	fptr = fopen(fname, "r");
	
	int i = 0;
	int tot = 0;
	while (fscanf(fptr, "%lu %d %d", &number, &tech, &is_paid) > 0) {
		accounts[i].subscriber_num = number;
		accounts[i].tech = tech;
		accounts[i].is_paid = is_paid;
		i++;
	}
	fclose(fptr);
	tot = i;
	return tot;
}

int main(int argc, char *argv[]) {
	
	int sock, portno, n, num_accounts, check_result;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	data_packet request_p;
	data_packet respond_p;
	account accounts[DB_size];
	
	if (argc < 2) {
		print_error("Error! No port provided.\n");
		exit(1);
	}
	
	// create a udp socket
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) { 
		print_error("Error while creating socket.\n");
		exit(1);
		}
	
	bzero((char *) &server_addr, sizeof(server_addr));
	portno = atoi(argv[1]);
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(portno);
	
	// Bind socket descriptor to the server address
	bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
	
	num_accounts = loadDB("Verification_Database.txt", accounts);
	
	printf("\nServer: Listening on port %d...\n\n", portno);
	
	while (1) {
		recvfrom(sock, &request_p, sizeof(data_packet), 0, (struct sockaddr *) &client_addr, &addrlen);
		
		printf("\nServer: Access request received\n");
		printf("Subscriber %lu with %dG\n", request_p.subscriber_num, request_p.tech);
		
		check_result = checkDB(request_p.subscriber_num, request_p.tech, accounts, num_accounts);
				
		respond_p.start_id = START_ID;
		respond_p.cli_id = request_p.cli_id;
		respond_p.seg_num = request_p.seg_num;
		respond_p.length = MAX_LENGTH;
		respond_p.tech = check_result;
		respond_p.subscriber_num = request_p.subscriber_num;
		respond_p.end_id = END_ID;
		
		if (check_result == 0) {
			respond_p.type = NOT_EXIST;
			print_error("Server: Subscriber does not exist on database.\n");
		} else if (check_result == 1) {
			respond_p.type = NOT_PAID;
			print_error("Server: Subscriber has not paid.\n");
		} else {
			respond_p.type = ACC_OK;
			printf("Server: Access permitted\n");
		}
		
		// sending response message
		sendto(sock, &respond_p, sizeof(data_packet), 0, (struct sockaddr *) &client_addr, addrlen);
	}
	
}