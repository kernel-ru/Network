/*
 * ping.c
 * written by Ivan Ryabtsov ivriabtsov at gmail dot com
 * compile with: gcc -Wall -Wextra -g -O2 -DDEBUG=2 ping.c for maximum debug
 * gcc -Wall -Wextra -g -O2 -DDEBUG=1 ping.c for reduce debug
 * gcc -Wall -Wextra -g -O2  ping.c without debug
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

#if DEBUG > 0
#define pr_dbg(...) fprintf(stderr, __VA_ARGS__)
#else
#define pr_dbg(...) ;
#endif

struct stuff {
	int recv_sock, send_sock;
	struct sockaddr_in recv_sockaddr, send_sockaddr;
	struct hostent *h_ent;
};

int ping(const char *name);
int routines(struct stuff *conn);
unsigned short in_cksum(unsigned short *addr, size_t len);
void pr_bytes(const char *str, int size);

int main(int argc, char *argv[])
{
	pr_dbg("pr_dbg is works\n");
	if (argc < 2) {
		fprintf(stderr, "need hostname\n");
		exit(1);
	}
	if (ping(argv[1])) {
		printf("host %s is alive\n", argv[1]);
		return 0;
	} else {
		printf("host %s is not alive\n", argv[1]);
		return 1;
	}
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
	/*pr_dbg("0x%X\n", send_sockaddr.sin_addr.s_addr);*/
	if ( (conn->recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) ==
			-1) {
		perror("socket: ");
		return 0;
	}
	conn->recv_sockaddr.sin_family = AF_INET;
	conn->recv_sockaddr.sin_addr.s_addr = *(long *)conn->
		h_ent->h_addr_list[0];
	/*pr_dbg("0x%X\n", recv_sockaddr.sin_addr.s_addr);*/
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
		/* comparison src and dest addresses */
		if ( *(unsigned int *)conn->h_ent->h_addr_list[0] ==
				((struct iphdr *)recv_frame)->saddr) {
			pr_dbg("package with a valid addresses\n");
		} else {
			pr_dbg("!!!!!src and dest addresses is not walid\n");
			goto cont;
		}
		iphdrlen = ((struct iphdr *)recv_frame)->ihl * sizeof(int);

		/* check chksum in ip header */
		cksum = ((struct iphdr *)recv_frame)->check;
		((struct iphdr *)recv_frame)->check = 0;
		if (cksum == in_cksum((unsigned short *)recv_frame, iphdrlen)){
			pr_dbg("ip cksum is equal %hu\n", cksum);
		} else {
			pr_dbg("!!!!!ip cksum is not equal %hu\n", cksum);
			goto cont;
		}

		/* compute len of icmp header */
		icmplen = ntohs(((struct iphdr *)recv_frame)->tot_len) -
			iphdrlen;

		/* check chksum of icmp header */
		cksum = ((struct icmphdr *)(recv_frame + iphdrlen))->
			checksum;
		((struct icmphdr *)(recv_frame + iphdrlen))->checksum = 0;
		if (cksum == in_cksum((unsigned short *)
					(recv_frame + iphdrlen), icmplen)) {
			pr_dbg("icmp cksum is equal %d\n", cksum);
		} else {
			pr_dbg("!!!!!icmp checksum is not walid %hu\n", cksum);
			goto cont;
		}

		/* check type and code */
		if (((struct icmphdr *)(recv_frame + iphdrlen))->type ==
				ICMP_ECHOREPLY && ((struct icmphdr *)
					(recv_frame + iphdrlen))->code
				== ICMP_ECHOREPLY ) {
			pr_dbg("icmp type and code is ICMP_ECHOREPLY\n");
		} else {
			pr_dbg("!!!!!icmp is not ICMP_ECHOREPLY\n");
			pr_dbg("!!!!!type: %hu, code: %hu\n",
					((struct icmphdr *)(recv_frame +
						iphdrlen))->type,
					((struct icmphdr *)(recv_frame +
						iphdrlen))->code);
		}

		/* check id */
		if (((struct icmphdr *)(recv_frame + iphdrlen))->un.echo.id ==
				send_frame->_icmphdr.un.echo.id) {
			pr_dbg("recv and send id is equal %hu\n",
					ntohs(send_frame->_icmphdr.un.echo.id));
		} else {
			pr_dbg("!!!!!id is not equal %hu\n",
					ntohs(send_frame->_icmphdr.un.echo.id));
			goto cont;
		}

		/* check sequence */
		if (((struct icmphdr *)(recv_frame + iphdrlen))->
				un.echo.sequence ==
				send_frame->_icmphdr.un.echo.sequence) {
			pr_dbg("recv end send seq is equal %hu\n",
					ntohs(send_frame->
						_icmphdr.un.echo.sequence));
		} else {
			pr_dbg("!!!!!sequence is not equal l %hu, r %hu\n",
					ntohs(send_frame->
						_icmphdr.un.echo.sequence),
					htons(((struct icmphdr *)
							(recv_frame + iphdrlen))
						->un.echo.sequence));
		}

		/* increment the counter of successful pings */
		succ_cnt++;
		pr_dbg("count of success ping: %d\n", succ_cnt);
cont:	
		seqtmp = ntohs(send_frame->_icmphdr.un.echo.sequence);
		seqtmp++;
		send_frame->_icmphdr.un.echo.sequence = htons(seqtmp);
		/* print all frame if DEBUG > 1 */
#if DEBUG == 2
		pr_bytes(recv_frame, ntohs(((struct iphdr *)recv_frame)->
					tot_len));
#endif
		pr_dbg("next sequence is %hu\n=============\n", seqtmp);
	}
	return succ_cnt;
}

void pr_bytes(const char *str, int size)
{
	unsigned char hex[] = {'0','1','2','3','4','5',
		'6','7','8','9','a','b','c','d','e','f'};
	char out[3];
	out[2] = '\0';
	unsigned char tmp;
	int i, c;
	for (i = 0; i < size; i++) {
		tmp = str[i] & 0xff;
		for (c = 1; c >= 0; c--) {
			out[c] = hex[tmp % 16];
			tmp /= 16;
		}
		if (i % 8 == 0 && i != 0) {
			if (i % 16 == 0) {
				fprintf(stderr, "\n");
			} else {
				fprintf(stderr, "   ");
			}
		}
		fprintf(stderr, "%s ", out);
	}
	fprintf(stderr, "\n");
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

