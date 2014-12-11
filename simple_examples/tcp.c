#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#define D_PORT 80

struct pseudohdr{
	unsigned int src;
	unsigned int dst;
	unsigned char filler; /* always null */
	unsigned char proto; /* always 0x6 */
	unsigned short len; /* len of real tcp hdr + data */
};

unsigned short in_cksum(unsigned short *addr, size_t len);

int main(int argc, char *argv[])
{
	struct addrinfo *hints, *answ, *tmp;
	struct pseudohdr *psehdr;
	struct tcphdr *send_frame;
	void *final_frame;
	int sock, ret;
	ssize_t send_cnt;
	unsigned short s_port, d_port = htons(D_PORT);
	unsigned int seq_num, ack_num;
	time_t t;
	srand((unsigned int)time(&t));
	s_port = htons((rand() % USHRT_MAX - 2048 + 1) + 2048);
	d_port = htons(D_PORT);
	seq_num = htonl(rand() % UINT_MAX + 1);
	ack_num = 0;
	if (argc < 2) {
		fprintf(stderr, "need host name\n");
		exit(1);
	}
	if ( (hints = calloc(1, sizeof(struct addrinfo))) == NULL) {
		perror("calloc");
		exit(1);
	}
	if ( (send_frame = calloc(1, sizeof(struct tcphdr))) == NULL) {
		perror("malloc");
		exit(1);
	}
	if ( (psehdr = calloc(1, sizeof(struct pseudohdr))) == NULL) {
		perror("malloc");
		exit(1);
	}
	if ( (final_frame = calloc(1, sizeof(struct tcphdr) +
					sizeof(struct pseudohdr))) == NULL) {
		perror("malloc");
		exit(1);
	}
	hints->ai_family = AF_INET;
	hints->ai_socktype = SOCK_RAW;
	hints->ai_protocol = IPPROTO_TCP;
	if ( (ret = getaddrinfo(argv[1], NULL, hints, &answ)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(1);
	}
	for (tmp = answ; tmp != NULL; tmp = tmp->ai_next) {
		if ( (sock = socket(tmp->ai_family, tmp->ai_socktype,
						tmp->ai_protocol)) == -1) {
			perror("socket");
			if (tmp == NULL) { /*walk around the whole list*/
				fprintf(stderr, "failed open socket\n");
				exit(1);
			} else {
				continue;
			}
		} else {
			printf("was openned sock with addr: %s\n",
					inet_ntoa(((struct sockaddr_in *)tmp->
						ai_addr)->sin_addr) );
			break;
		}
	}
	/*old method:
	if ( (sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		exit(1);
	}
	*/
	/* get my ip address */
	connect(sock, tmp->ai_addr, tmp->ai_addrlen);
	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);
	getsockname(sock, (struct sockaddr *)&addr, &len);
	printf("tcp from address: %s\n", inet_ntoa(addr.sin_addr));

	/* filling pseudo hdr */
	psehdr->src = addr.sin_addr.s_addr;
	psehdr->dst = ((struct sockaddr_in *)tmp->ai_addr)->sin_addr.s_addr;
	psehdr->filler = 0;
	psehdr->proto = IPPROTO_TCP;
	psehdr->len = htons(20);

	/* prepare tcp hdr */
	send_frame->source = s_port;
	send_frame->dest = d_port;
	send_frame->seq = seq_num;
	send_frame->ack_seq = ack_num;
	send_frame->doff = 5;
	send_frame->fin = 0;
	send_frame->syn = 1;
	send_frame->rst = 0;
	send_frame->psh = 0;
	send_frame->ack = 0;
	send_frame->urg = 0;
	send_frame->window = 0xffff;
	send_frame->check = 0;

	/* collect all frame (pseudo and real hdr) for computing checksum */
	memcpy(final_frame, psehdr, sizeof(struct pseudohdr));
	memcpy(final_frame + sizeof(struct pseudohdr), send_frame,
			sizeof(struct tcphdr));
	send_frame->check = in_cksum(final_frame, sizeof(struct tcphdr)+
			sizeof(struct pseudohdr));

	/* send tcp hdr */
	send_cnt = sendto(sock, send_frame, sizeof(struct tcphdr), 0,
			tmp->ai_addr, tmp->ai_addrlen);
	if (send_cnt == -1) {
		perror("sendto");
		exit(1);
	} else {
		printf("was send %d bytes\n", send_cnt);
	}
	return 0;
}

unsigned short in_cksum(unsigned short *addr, size_t len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;
	
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}
	
	if (nleft == 1) {
		*(unsigned char *)(&answer) += *(unsigned char *)w;
		sum += answer;
	}
	
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}
