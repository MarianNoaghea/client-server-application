#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include "header.h"

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}


int main(int argc, char *argv[])
{	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 3) {
		usage(argv[0]);
	}

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	FD_ZERO(&tmp_fds);
	FD_ZERO(&read_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = sockfd;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = atoi(argv[3]);
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	msg_srv* m =(msg_srv*) calloc(1, sizeof(msg_srv));

	// la prima conectare se trimite id-ul clientului catre server
	n = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(n < 0, "send");
	while (1) {
  		// se citeste de la tastatura
		tmp_fds = read_fds;
		select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		if(FD_ISSET(0, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				free(m);
				break;
			}

			n = send(sockfd, buffer, strlen(buffer), 0);
			DIE(n < 0, "send");
		} else {
			memset(m, 0, sizeof(msg_srv));
			memset(&(m->mesaj), 0, sizeof(msg));
			recv(sockfd, &(m->size), 3 * sizeof(int), 0);
			n = recv(sockfd, &m->mesaj, m->size, 0);

			DIE(n < 0, "recv");
			if (strncmp(m->mesaj.topicName, "Subscribed", 9) == 0) {
					printf("%s\n", m->mesaj.topicName);

			} else if (strncmp(m->mesaj.topicName, "Unsubscribed", 11) == 0) {
					printf("%s\n", m->mesaj.topicName);

			} else if (strncmp(m->mesaj.topicName,
			 					"You're already subscribed to this topic.", 16) == 0) {
				printf("%s\n", m->mesaj.topicName);

			} else if (strncmp(m->mesaj.topicName,
								"You're already connected from another process.", 16) == 0) {
				printf("Esti deja conectat\n");
				break;

			} else if (strncmp(m->mesaj.topicName, "Server is closed.", 16) == 0) {
				free(m);
				exit(0);

			} else if (m->size > 0) {
				process_payload(m);
			}
		}
	}

	close(sockfd);

	return 0;
}
