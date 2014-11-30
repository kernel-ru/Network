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
#include <string.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

#if DEBUG > 0
#define pr_dbg(...) fprintf(stderr, __VA_ARGS__)
#else
#define pr_dbg(...) ;
#endif

#define MAX_NAME_SIZ 256
#define DELAY 500000
#define BUF_SIZ 1000
#define LOAD_SIZ 5

struct stuff {
	useconds_t delay;
	char hname[MAX_NAME_SIZ];
	int recv_sock, send_sock;
	struct sockaddr_in recv_sockaddr, send_sockaddr;
	struct hostent *h_ent;
};

struct s_frame {
	struct icmphdr _icmphdr;
	char payload[LOAD_SIZ];
};

void usage(int st);
int ping4(struct stuff *conn);
int ping6(struct stuff *conn);
int routines(struct stuff *conn);
unsigned short in_cksum(unsigned short *addr, size_t len);
void pr_bytes(const char *str, int size);
void sighndlr(int sig);

jmp_buf env;

int main(int argc, char *argv[])
{
	int res, i;
	struct stuff *conn = NULL;
	const char optstring[] = "hp:qd:H:";
	char q_fl = 0, h_fl = 0 proto = 4;
	if (argc < 3) {
		usage(0);
		return 1;
	}
	conn = malloc(sizeof(struct stuff));
	if (!conn) {
		perror("malloc: ");
		return 1;
	}
	conn->delay = DELAY;
	for (;;) {
		res = getopt(argc, argv, optstring);
		if (res == -1) {
			break;
		}
		switch (res) {
			case 'd':
				for (i = 0; optarg[i]; i++) {
					if (!isdigit((int)optarg[i] & 0xff)) {
						usage(0);
						res = 1;
						goto exit;
					}
				}
				conn->delay = atoi(optarg);
				break;
			case 'h':
				usage(1);
				goto exit;
				break;
			case 'H':
				strcpy(conn->hname, optarg);
				h_fl = 1;
				break;
			case 'p':
				if (strcmp(optarg, "6") == 0 ||
						strcmp(optarg, "4") == 0) {
					proto = atoi(optarg);
				} else {
					res = 1;
					usage(0);
					goto exit;
				}
				break;
			case 'q':
				q_fl = 1;
				break;
			case '?':
				fprintf(stderr, "unrecognized option: %c\n",
						res);
				res = 1;
				usage(0);
				goto exit;
				break;
			default:
				fprintf(stderr, "getopt return code 0x%x\n",
						res & 0xff);
				res = 1;
				usage(0);
				goto exit;
				break;
		}
	}
	if (!h_fl) {
		usage(0);
		res = 1;
		goto exit;
	}
	if (proto == 6) {
		fprintf(stderr, "ipv6 under construction\n");
		res = 1;
		goto exit;
		if (ping6(conn))
			goto succ;
		else
			goto fail;
	} else {
		if (ping4(conn))
			goto succ;
		else
			goto fail;
	}
succ:
	res = 0;
	if (q_fl == 0) {
		printf("host %s is alive\n", conn->hname);
	}
	goto exit;
fail:
	res = 1;
	if (q_fl == 0) {
		printf("host %s is not alive\n", conn->hname);
	}
exit:
	free(conn);
	exit(res);
}

int ping4(struct stuff *conn)
{
	struct sigaction act;
	void *succ_cnt;
	pthread_t routin_th;
	int th_result;
	conn->h_ent = gethostbyname(conn->hname);
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
	if ( (conn->recv_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) ==
			-1) {
		perror("socket: ");
		return 0;
	}
	conn->recv_sockaddr.sin_family = AF_INET;
	conn->recv_sockaddr.sin_addr.s_addr = *(long *)conn->
		h_ent->h_addr_list[0];
	/* set sighandler */
	memset(&act, 0, sizeof(act));
	act.sa_handler = sighndlr;
	sigaction(SIGUSR1, &act, 0);
	/* start pthread */
	th_result = pthread_create(&routin_th, NULL, (void *)&routines,
			(void *)conn);
	if (th_result != 0) {
		perror("!!!!!ptread: ");
		return 0;
	}
	usleep(conn->delay);
	/* send SIGUSR1 to thread */
	pthread_kill(routin_th, SIGUSR1);
	/* get the number of sucessful ping */
	if (pthread_join(routin_th, &succ_cnt) == 0) {
		if (((long)succ_cnt) >= 3) {
			pr_dbg("total ping recieved: %lu\n", (long)succ_cnt);
			return 1;
		} else {
			pr_dbg("total ping recieved: %lu\n", (long)succ_cnt);
			return 0;
		}
	} else {
		perror("pthread_joint: ");
		return 0;
	}
}

void sighndlr(int sig)
{
	pr_dbg("it is sig handler, signal was received %d\n", sig);
	longjmp(env, 1);
}

int routines(struct stuff *conn)
{
	if (setjmp(env) == 0) {
		pr_dbg("setjmp\n");
	} else {
		pr_dbg("return from longjmp\n");
		goto end;
	}
	sigset_t set, orig;
	struct s_frame *send_frame = NULL;
	void *recv_frame = NULL;
	void *succ_cnt = 0;
	int i;
	unsigned short cksum, seqtmp;
	time_t t;
	int iphdrlen, icmplen;
	socklen_t len = sizeof(struct sockaddr_in);
	srand((unsigned int)time(&t));
	send_frame = malloc(sizeof(struct s_frame));
	if (!send_frame) {
		perror("malloc: ");
		goto end;
	}
	recv_frame = malloc(BUF_SIZ);
	if (!recv_frame) {
		perror("malloc: ");
		goto end;
	}
	send_frame->_icmphdr.type = ICMP_ECHO;
	send_frame->_icmphdr.code = ICMP_ECHOREPLY;
	send_frame->_icmphdr.un.echo.id = rand() % (USHRT_MAX + 1);
	send_frame->_icmphdr.un.echo.sequence = htons(1);
	for (i = 0; i < 4; i++) {
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
		memset(recv_frame, 0, BUF_SIZ);
		if (recvfrom(conn->recv_sock, recv_frame, BUF_SIZ,
					MSG_WAITALL,
					(struct sockaddr *)&conn->recv_sockaddr,
					&len) == -1) {
			perror("!!!!!recvfrom: ");
			goto cont;
		}

		/* compute len of ip header */
		iphdrlen = ((struct iphdr *)recv_frame)->ihl * sizeof(int);

		/* compute len of icmp header */
		icmplen = ntohs(((struct iphdr *)recv_frame)->tot_len) -
			iphdrlen;

		/* check chksum in ip header */
		cksum = ((struct iphdr *)recv_frame)->check;
		((struct iphdr *)recv_frame)->check = 0;
		if (cksum == in_cksum((unsigned short *)recv_frame, iphdrlen)){
			pr_dbg("ip cksum is equal %hu\n", cksum);
		} else {
			pr_dbg("!!!!!ip cksum is not equal %hu\n", cksum);
			goto cont;
		}

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

		/* comparison src and dest addresses */
		if ( *(unsigned int *)conn->h_ent->h_addr_list[0] ==
				((struct iphdr *)recv_frame)->saddr) {
			pr_dbg("package with a valid addresses\n");
		} else {
			pr_dbg("!!!!!src and dest addresses is not walid\n");
			goto cont;
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
		pr_dbg("count of success ping: %lu\n", (long)succ_cnt);
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

end:
	/* set block for SIGUSR1 */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigemptyset(&orig);
	pthread_sigmask(SIG_BLOCK, &set, &orig);
	free(send_frame);send_frame = NULL;free(recv_frame);recv_frame = NULL;
	/* unset block for SIGUSR1 */
	pthread_sigmask(SIG_SETMASK, &orig, 0);
	pthread_exit(succ_cnt);
}

#if DEBUG > 1
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
#endif

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

void usage(int st)
{
	if (st) 
		printf("usage:./a.out [-h] -H localhost [-p 4|6] [-q] [-d]\n");
	else
		fprintf(stderr, "usage:./a.out [-h] -H localhost [-p 4|6] [-q] [-d delay in nanosec]\n");
}
