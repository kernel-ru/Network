#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

int main(int argc, char *argv[])
{
	struct addrinfo *hints, *answ, *tmp;
	struct tcphdr *send_frame;
	int sock, ret;
	ssize_t send_cnt;
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
	send_cnt = sendto(sock, send_frame, sizeof(struct tcphdr), 0,
			tmp->ai_addr, tmp->ai_addrlen);
	if (send_cnt == -1) {
		perror("sendto");
		exit(1);
	}
	return 0;
}
