#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>


int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "need argument\n");
		return 1;
	}
	struct addrinfo hints, *result, *tmp;
	struct sockaddr_in *sock_addr;
	int res;
	hints.ai_flags = 0;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = 0;
	hints.ai_addrlen = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	//hints.ai_protocol = IPPROTO_ICMP;

	if ((res = getaddrinfo(argv[1], NULL, &hints, &result)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
		return 1;
	}

	for (tmp = result; tmp; tmp = tmp->ai_next) {
		/*
		sockfd = socket(tmp->ai_family, tmp->ai.socktype, tmp->
				ai_protocol);
		if (sockfd == -1)
			continue;
		*/
		sock_addr = (struct sockaddr_in *)tmp->ai_addr;
		printf("address is: %s\n", inet_ntoa(sock_addr->sin_addr));
	}
	freeaddrinfo(result);
	return 0;
}
