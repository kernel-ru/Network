#include <error.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <resolv.h>
#include <netinet/ip_icmp.h>

#define PACKETSIZE  64

int pid=-1;
struct protoent *proto=NULL;

struct packet
{
    struct icmphdr hdr;
    char msg[PACKETSIZE-sizeof(struct icmphdr)];
};


void usage()
{
    printf("USAGE:\t  ccns <IP_address>\n");
    exit(1);
}


unsigned short checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;

    for(sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if(len == 1)
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int ping(char *host)
{
    struct hostent *hname;
    struct sockaddr_in addr;

    proto = getprotobyname("ICMP");
    hname = gethostbyname(host);

    bzero(&addr, sizeof(addr));

    addr.sin_family = hname->h_addrtype;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = *(long*)hname->h_addr;

    const int val=255;
    int i, sd, cnt=0;
    struct packet pckt;
    struct sockaddr_in r_addr;

    sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sd < 0) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
        perror("Set TTL option");

    if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0 )
        perror("Request nonblocking I/O");

    int count, icmp = 0;
    for(count = 0; count < 4; count++) {
        int len = sizeof(r_addr);

        bzero(&pckt, sizeof(pckt));
        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = pid;

        for(i = 0; i < sizeof(pckt.msg)-1; i++)
            pckt.msg[i] = i+'0';

        pckt.msg[i] = 0;
        pckt.hdr.un.echo.sequence = cnt++;
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

		printf("Msg #%d\n", cnt);
        if (sendto(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&addr, sizeof(addr)) <= 0 )
            perror("sendto");

        sleep(1);

        if (recvfrom(sd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&r_addr, &len) > 0 ) {
            icmp++;
			printf("***Got message!***\n");
            if (icmp > 1)
                return icmp;
        }

	}
    return icmp;
}

int main(int argc, char *argv[])
{
    if(argv[1] == NULL)
        usage();

    if (ping(argv[1]) > 0)
		printf("PING\t\tOK\n");
    else
		printf("PING\t\tNO_ANSWER\n");
    
	return 0;
}

