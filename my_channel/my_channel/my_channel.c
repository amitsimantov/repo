#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE 
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define __STDC_WANT_LIB_EXT2__ 1
#define _GNU_SOURCE

#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>

#include <conio.h>
#include <stdlib.h>



#pragma comment(lib, "ws2_32.lib")
#define SUCCESS 0
#define FAILURE -1
#define MAX_CLIENTS 5
#define BUF_LEN 1500



// The following definitions are used for bit extractions from char arrays
// Source: https://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)BitExtraction.html

#define BITS_PER_BYTE (8)
/* extract the n-th bit of x */
#define GET_BIT(x, n) ((((x)[(n) / BITS_PER_BYTE]) & (0x1 << ((n) % BITS_PER_BYTE))) != 0)
/* set the n-th bit of x to 1 */
#define SET_BIT(x, n) ((x)[(n) / BITS_PER_BYTE]) |= (0x1 << ((n) % BITS_PER_BYTE))
/* set the n-th bit of x to 0 */
#define RESET_BIT(x, n) ((x)[(n) / BITS_PER_BYTE]) &= ~(0x1 << ((n) % BITS_PER_BYTE))

int total_flipped_bits = 0;

void _error(char* error_msg) {
	fprintf(stderr, "Error: %s - %s", error_msg, strerror(errno));
	exit(FAILURE);
}

void print_message(char* msg, int len) {
	int i, j, bit;
	for (i = 0; i < len; i++) {
		for (j = 0; j < 8; j++) {
			bit = GET_BIT(msg, i * 8 + j);
			printf("%d", bit);
		}
		printf(" ");
	}
}

//int add_noise_to_msg(int seed, int n, char* msg, int len) {
//	int i, j, flipped = 0, bit;
//	double k, p = n / pow(2, 16);
//	printf("\nError Probability: %lf\n", p);
//	printf("\nError Percentage=%lf\n", p*100);
//	srand(seed);
//	for (i = 0; i < len; i++) {
//		for (j = 0; j < 8; j++) {
//			k = (double)rand() / RAND_MAX;
//			if (k < p) {
//				//printf("\nrand()/RANDMAX=%lf < p=%lf\n", k, p);
//				printf("########################  ON BYTE %d FLIPPING BIT %d###################### \n\n", i, j);
//				bit = GET_BIT(msg, i * 8 + j);
//				if (bit) {
//					//printf("bit was: %d\n", bit);
//					RESET_BIT(msg, i * 8 + j);
//					//printf("and now bit is: %d\n", GET_BIT(msg, i * 8 + j));
//				}
//				else {
//					//printf("bit was: %d\n", bit);
//					SET_BIT(msg, i * 8 + j);
//					//printf("and now bit is: %d\n", GET_BIT(msg, i * 8 + j));
//				}
//				flipped++;
//				//printf("###################### FLIPPED COUNTER: {%d}  ######################## \n\n", flipped);
//			}
//			//else
//				//printf("\nrand()/RANDMAX=%lf > p=%lf ^^^^^^^^^ NO FLIPPING ^^^^^^^^\n", k, p);
//		}
//	}
//	//printf("Flipped: %d bits", flipped);
//	return flipped;
//}


int add_noise_to_msg(int seed, int n, char* msg, int len) {
	int i, j, flipped = 0, bit;
	double k, p = n / pow(2, 16);
	srand(seed);
	for (i = 0; i < ((len/15)*8); i++) {
		for (j = 0; j < 15; j++) {
			k = (double)rand() / RAND_MAX;
			if (k < p) {
				//printf("\nrand()/RANDMAX=%lf < p=%lf\n", k, p);
				//printf("########################  ON BLOCK %d FLIPPING BIT %d###################### \n\n", i, j+1);
				bit = GET_BIT(msg, i * 15 + j);
				if (bit) {
					//printf("bit was: %d\n", bit);
					RESET_BIT(msg, i * 15 + j);
					//printf("and now bit is: %d\n", GET_BIT(msg, i * 8 + j));
				}
				else {
					//printf("bit was: %d\n", bit);
					SET_BIT(msg, i * 15 + j);
					//printf("and now bit is: %d\n", GET_BIT(msg, i * 8 + j));
				}
				flipped++;
				//printf("###################### FLIPPED COUNTER: {%d}  ######################## \n\n", flipped);
			}
			//else
				//printf("\nrand()/RANDMAX=%lf > p=%lf ^^^^^^^^^ NO FLIPPING ^^^^^^^^\n", k, p);
		}
	}
	//printf("Flipped: %d bits", flipped);
	return flipped;
}

int main(int argc, char* argv[]) {
	// initialize windows networking
	int ch_port, sender_port, receiver_port, n, seed, counter, recv, sent, flipped = 0, status, ch, total_recv = 0;
	char* receiver_ip = argv[2], buf[BUF_LEN], small_buffer[256];
	char* sender_ip = (char*)malloc(256);
	if (sender_ip == NULL)
		_error("malloc() failed!\n");
	
	sscanf(argv[1], "%d", &ch_port);
	//sscanf(argv[2], "%s", &dest_ip);
	sscanf(argv[3], "%d", &receiver_port);
	sscanf(argv[4], "%d", &n);
	sscanf(argv[5], "%d", &seed);

	//fprintf(stderr, "Channel_port: %d\n", htons(ch_port));
	//fprintf(stderr, "Dest_ip: %s\n", receiver_ip);
	//fprintf(stderr, "Dest_port: %d\n", htons(receiver_port));
	//fprintf(stderr, "N: %d\n", n);
	//fprintf(stderr, "Seed: %d\n", seed);

	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
		printf("Error at WSAStartup()\n");

	fd_set readfds, writefds;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	struct timeval timeout = { 0, 500 };

	SOCKET sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

	struct sockaddr_in ch_addr, recv_addr, src_addr, *sender_addr;
	sender_addr = NULL;

	ch_addr.sin_family = AF_INET;
	ch_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ch_addr.sin_port = htons(ch_port);

	recv_addr.sin_family = AF_INET;
	recv_addr.sin_addr.s_addr = inet_addr(receiver_ip);
	recv_addr.sin_port = htons(receiver_port);

	status = bind(sock_fd, (SOCKADDR*) &ch_addr, sizeof(ch_addr));
	if (status == SOCKET_ERROR)
		_error("failed in binding socket\n");

	int s_size = sizeof(struct sockaddr_in);
	
	while (1) {

		FD_SET(sock_fd, &readfds);
		memset(buf, '\0', BUF_LEN);
		status = select(sock_fd + 1, &readfds, &writefds, NULL, &timeout);  
		if (status == SOCKET_ERROR) {
			_error("select() returned SOCKET_ERROR\n");
		}
		
		if (FD_ISSET(sock_fd, &readfds)) {
			
			if ((recv = recvfrom(sock_fd, buf, BUF_LEN, 0, (SOCKADDR*) &src_addr, &s_size)) > 0) {
				if (sender_addr == NULL) {  // save sender address 
					sender_addr = (SOCKADDR_IN*)malloc(sizeof(SOCKADDR_IN));
					if (sender_addr == NULL)
						_error("malloc() failed!\n");
					sender_addr->sin_port = src_addr.sin_port;
					sender_addr->sin_family = src_addr.sin_family;
					sender_addr->sin_addr = src_addr.sin_addr;
					*sender_addr = src_addr;
					sender_port = ntohs(sender_addr->sin_port);
					sender_ip = inet_ntoa(sender_addr->sin_addr);
					//printf("First Message from Sender: ip=%s, port=%d\n", sender_ip, sender_addr->sin_port);
					total_recv += recv;
					
				}

				if (src_addr.sin_addr.s_addr == inet_addr(receiver_ip) && src_addr.sin_port == htons(receiver_port)) {
					//fprintf(stderr, "\nGot %d bytes from RECEIVER:  port=%d\n", recv, src_addr.sin_port);
					//printf("%s\n", buf);
					sent = sendto(sock_fd, buf, recv, 0, sender_addr, &s_size);
					//fprintf(stderr, "\nCHANNEL sent: %d bytes to SENDER\n", sent);
					memset(small_buffer, '\0', 256);
					sprintf(small_buffer, "Sender: %s\nReceiver: %s\n%d Total bytes\nFlipped: %d bits\n", sender_ip, receiver_ip, total_recv, total_flipped_bits);
					fprintf(stderr, small_buffer);
					status = closesocket(sock_fd);
					if (status == SOCKET_ERROR)
						_error("failed in closing socket\n");
					exit(SUCCESS);
				}

				else {
					//fprintf(stderr, "Got {%d} bytes from SENDER: port={%d}\n", recv, src_addr.sin_port);
					flipped = add_noise_to_msg(seed, n, buf, recv);
					total_flipped_bits += flipped;
					sent = sendto(sock_fd, buf, recv, 0, (SOCKADDR*)&recv_addr, &s_size);
					//fprintf(stderr, "\nCHANNEL sent: %d bytes to RECEIVER - FLIPPED TOTAL %d bits\n", sent , total_flipped_bits);
					//print_message(buf, recv);
					}
				}
			}

		}
		
			

	

}

