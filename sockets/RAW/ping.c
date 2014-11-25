/*
 * ping.c
 * written by Ivan Ryabtsov ivriabtsov at gmail dot com
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <string.h>

struct stuff {
	int recv_sock, send_sock;
	struct sockaddr_in recv_sockaddr, send_sockaddr;
	struct hostent *h_ent;
};

int ping(const char *name);
int routines(struct stuff *conn);
unsigned short in_cksum(unsigned short *addr, size_t len);

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("need hostname\n");
		exit(1);
	}
	if (ping(argv[1])) {
		printf("host %s is alive\n", argv[1]);
	} else {
		printf("host %s is not alive\n", argv[1]);
	}
	return 0;
}


int ping(const char *name)
{
	struct stuff *conn;
	conn = malloc(sizeof(struct stuff));
	conn->h_ent = gethostbyname(name);
	if (!conn->h_ent) {
		perror("gethostbyname: ");
		return 0;
	}
	if ( (conn->send_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) ==
			-1) {
		perror("socket: ");
		return 0;
	}
	conn->send_sockaddr.sin_family = AF_INET;
	conn->send_sockaddr.sin_addr.s_addr =
		*(in_addr_t *)conn->h_ent->h_addr_list[0];
	/*printf("0x%X\n", send_sockaddr.sin_addr.s_addr);*/
	if ( (conn->recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) ==
			-1) {
		perror("socket: ");
		return 0;
	}
	conn->recv_sockaddr.sin_family = AF_INET;
	conn->recv_sockaddr.sin_addr.s_addr = *(long *)conn->
		h_ent->h_addr_list[0];
	/*printf("0x%X\n", recv_sockaddr.sin_addr.s_addr);*/
	if (routines(conn) >= 3) {
		return 1;
	} else {
		return 0;
	}
}

#define BUF_SIZ 100
#define LOAD_SIZ 5
struct s_frame {
	struct icmphdr _icmphdr;
	char payload[LOAD_SIZ];
};
int routines(struct stuff *conn)
{
	struct s_frame *send_frame;
	void *recv_frame;
	unsigned short cksum, seqtmp;
	int i, succ_cnt = 0;
	time_t t;
	int iphdrlen, icmplen;
	socklen_t len = sizeof(struct sockaddr_in);
	srand((unsigned int)time(&t));
	send_frame = malloc(sizeof(struct s_frame));
	if (!send_frame) {
		perror("malloc: ");
		return 0;
	}
	recv_frame = malloc(BUF_SIZ);
	if (!recv_frame) {
		perror("malloc: ");
		return 0;
	}
	send_frame->_icmphdr.type = ICMP_ECHO;
	send_frame->_icmphdr.code = ICMP_ECHOREPLY;
	/*send_frame->_icmphdr.un.echo.id = rand() % (USHRT_MAX + 1);*/
	send_frame->_icmphdr.un.echo.id = rand() % (USHRT_MAX + 1);
	send_frame->_icmphdr.un.echo.sequence = htons(1);
	/*send_frame->payload = rand() % (UINT_MAX + 1);*/
	for (i = 0; i < 4; i++) {
		memset(recv_frame, 0, 1400);
		send_frame->_icmphdr.checksum = 0;
		send_frame->_icmphdr.checksum =
			in_cksum((unsigned short *)send_frame,
					sizeof(struct s_frame));
		if (sendto(conn->send_sock, send_frame, sizeof(struct s_frame),
					0, (struct sockaddr *)&conn->
					send_sockaddr,
					sizeof(struct sockaddr_in)) == -1) {
			perror("!!!!!sendto: ");
			goto cont;
		}
		if (recvfrom(conn->recv_sock, recv_frame, BUF_SIZ,
					MSG_WAITALL,
					(struct sockaddr *)&conn->recv_sockaddr,
					&len) == -1) {
			perror("!!!!!recvfrom: ");
			goto cont;
		}
		/*sleep(1);*/
		/* comparison src and dest addresses */
		if ( *(unsigned int *)conn->h_ent->h_addr_list[0] ==
				((struct iphdr *)recv_frame)->saddr) {
			printf("package with a valid addresses\n");
		} else {
			printf("!!!!!src and dest addresses is not walid\n");
			goto cont;
		}
		/*
		printf("s 0x%X, d 0x%X\n", ((struct iphdr *)recv_frame)->saddr,
				((struct iphdr *)recv_frame)->daddr);
				*/
		iphdrlen = ((struct iphdr *)recv_frame)->ihl * sizeof(int);
		/* check chksum in ip header */
		cksum = ((struct iphdr *)recv_frame)->check;
		((struct iphdr *)recv_frame)->check = 0;
		if (cksum == in_cksum((unsigned short *)recv_frame, iphdrlen)){
			printf("ip cksum is equal %hu\n", cksum);
		} else {
			printf("!!!!!ip cksum is not equal %hu\n", cksum);
			goto cont;
		}
		/* compute len of icmp header */
		icmplen = ntohs(((struct iphdr *)recv_frame)->tot_len) -
			iphdrlen;
		/*printf("iphdr len: %d, icmp len: %d, all frame len: %hu\n",
				iphdrlen, icmplen,
				ntohs(((struct iphdr *)recv_frame)->tot_len));
		printf("%p, %p\n", recv_frame, recv_frame + iphdrlen);
		printf("%p, %p\n", recv_frame, (struct icmphdr *)(recv_frame +
				iphdrlen));
		printf("%d\n", ((struct icmphdr *)(recv_frame + iphdrlen))->
				checksum);*/
		/* check icmp id and sequence number */
		if (((struct icmphdr *)(recv_frame + iphdrlen))->un.echo.id ==
				send_frame->_icmphdr.un.echo.id) {
			printf("recv and send id is equal %hu\n",
					ntohs(send_frame->_icmphdr.un.echo.id));
		} else {
			printf("!!!!!id is not equal %hu\n",
					ntohs(send_frame->_icmphdr.un.echo.id));
			goto cont;
		}
		if (((struct icmphdr *)(recv_frame + iphdrlen))->
				un.echo.sequence ==
				send_frame->_icmphdr.un.echo.sequence) {
			printf("recv end send seq is equal %hu\n",
					ntohs(send_frame->
						_icmphdr.un.echo.sequence));
		} else {
			printf("!!!!sequence is not equal l %hu, r %hu\n",
					ntohs(send_frame->
						_icmphdr.un.echo.sequence),
					htons(((struct icmphdr *)
							(recv_frame + iphdrlen))
						->un.echo.sequence));
		//	goto cont;
		}

		/* check chksum of icmp header */
		cksum = ((struct icmphdr *)(recv_frame + iphdrlen))->
			checksum;
		/*printf("recv chsum: %hu\n", cksum);*/
		((struct icmphdr *)(recv_frame + iphdrlen))->checksum = 0;
		if (cksum == in_cksum((unsigned short *)
					(recv_frame + iphdrlen), icmplen)) {
			printf("icmp cksum is equal %d\n", cksum);
			succ_cnt++;
			printf("%d\n", succ_cnt);
		} else {
			printf("!!!!!icmp checksum is not walid %hu\n", cksum);
			goto cont;
		}
cont:	
		seqtmp = ntohs(send_frame->_icmphdr.un.echo.sequence);
		seqtmp++;
		send_frame->_icmphdr.un.echo.sequence = htons(seqtmp);
		printf("next sequence is %hu\n=============\n", seqtmp);
	}
	return succ_cnt;
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

