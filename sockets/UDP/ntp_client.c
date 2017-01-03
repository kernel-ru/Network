#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define oops(msg,x) {perror(msg);exit(x);}
#define VERSION "0.1"
#define NTP_TIMESTAMP_DELTA 2208988800ull

void usage(char** argv)
{
	printf("Yet another NTP client v%s\n", VERSION);
	printf("Usage:   %s <hostname OR ip_address>\n", *argv); //example: 64.113.32.5
	exit(1);
}

int main(int argc, char* argv[])
{
	if (argc != 2)
		usage(argv);

	typedef struct 
	{
		unsigned leap	: 2;
		unsigned ver	: 3;
		unsigned mode	: 3;
		
		uint8_t stratum;
		uint8_t poll;
		uint8_t precision;
		
		uint32_t root_delay;
		uint32_t root_dis;
		uint32_t ref_id;
		
		uint32_t ref_Tm_s;
		uint32_t ref_Tm_f;
		
		uint32_t orig_Tm_s;
		uint32_t orig_Tm_f;
		
		uint32_t rcv_Tm_s;
		uint32_t rcv_Tm_f;
		
		uint32_t tsm_Tm_s;
		uint32_t tsm_xTm_f;
	} ntp_packet;
	
	ntp_packet packet = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	*((char *) &packet + 0) = 0x1b;

	struct sockaddr_in serv_addr;
	struct hostent *server;
	int sockfd;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		oops("can't create socket", 2);
	
	if ((server = gethostbyname(*++argv)) == NULL)
		oops("can't find this host", 2);
	
	bzero((void *)&serv_addr, sizeof(serv_addr));
	bcopy((void *)server->h_addr, (void *)&serv_addr.sin_addr, server->h_length);
	
	serv_addr.sin_port = htons(123);
  serv_addr.sin_family = AF_INET;
	
	if (sendto(sockfd, &packet, sizeof(packet), 0,
						(struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
		oops("sendto failed", 3);

	socklen_t saddrlen = sizeof(serv_addr);	
	if (recvfrom(sockfd, (char*) &packet, sizeof(packet), 0,
              (struct sockaddr *) &serv_addr, &saddrlen) == -1)
		oops("recvfrom failed", 3)

	packet.tsm_Tm_s = ntohl(packet.tsm_Tm_s);
	time_t tTm = (time_t) (packet.tsm_Tm_s - NTP_TIMESTAMP_DELTA); 
	
	printf("Time: %s", ctime((const time_t*) &tTm));
	return 0;
}
