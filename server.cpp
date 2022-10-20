#include "helpers.h"
#include "header.h"

using namespace std;

void usage(char *file)
{	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char**argv)
{	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	if (argc < 2) {
		usage(argv[0]);
	}

	fd_set read_fds;  // multimea de citire folosita in select()
	fd_set tmp_fds;  // multime folosita temporar
	int fdmax;  // valoare maxima fd din multimea read_fds

	// se goleste multimea de descriptori de citire (read_fds)
	// si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	struct sockaddr_in cli_UDP_addr;
	char buf[BUFLEN];

	/*Deschidere socket UDP*/
	int sockUDP = socket(AF_INET, SOCK_DGRAM, 0);

	/*Setare struct sockaddr_in pentru a asculta pe portul respectiv */
	memset(&cli_UDP_addr, 0, sizeof(cli_UDP_addr));
	cli_UDP_addr.sin_family = AF_INET;
	cli_UDP_addr.sin_port = htons(atoi(argv[1]));
	cli_UDP_addr.sin_addr.s_addr = INADDR_ANY;

	/* Legare proprietati de socket */
	bind(sockUDP, (struct sockaddr*) &cli_UDP_addr, sizeof(cli_UDP_addr));
	memset(buf, 0, BUFLEN);
	int receive_UDP;
	socklen_t socklen_UDP = sizeof(cli_UDP_addr);

	FD_SET(sockUDP, &read_fds);

	int sockTCP, newSockTCP, portno;
	char bufferCLI[100];
	struct sockaddr_in serv_addr, cli_TCP_addr;
	int n = 0, i, ret;
	socklen_t clilen;

	sockTCP = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockTCP < 0, "socket");

	portno = htons(atoi(argv[1]));
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockTCP, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockTCP, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	FD_SET(0, &read_fds);
	FD_SET(sockTCP, &read_fds);  // singurul socket si multimea read
	fdmax = sockTCP;

	int conexiune = 0;
	int connect_same_id = 0;

	vector<client> clients;
	vector<topic*> topics;

	while (1) {
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		client c;

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockUDP) {
					// recv intr-o structura creata special
					msg* msgUDP = (msg*) calloc(1, sizeof(msg));
					receive_UDP = recvfrom(i, msgUDP, BUFLEN, 0,
					 						(struct sockaddr*) &(cli_UDP_addr),
											&socklen_UDP);
					DIE(receive_UDP < 0, "recvUDP");

					int ip = cli_UDP_addr.sin_addr.s_addr;
					int port = ntohs(cli_UDP_addr.sin_port);

					// trimit mai departe subscriberilor
					send_to_subscribers(topics, msgUDP, ip, port, receive_UDP);

					memset(msgUDP, 0, sizeof(msg));
					free(msgUDP);
				} else if (i == 0) {
					char bufferSTDIN[10];
					memset(bufferSTDIN, 0, 10);
					fgets(bufferSTDIN, 9, stdin);

					if (strncmp(bufferSTDIN, "exit", 4) == 0) {
						for (auto c : clients) {
							if (c.isConnected == 1) {
								msg_srv* m = generate_message("Server is closed.\n");
								int n = send(c.socket, m, m->size + 3 * sizeof(int), 0);
								DIE(n < 0, "send");
								free(m);
							}
						}

						for (auto t : topics) {
							free(t);
						}
						topics.clear();
						clients.clear();
						exit(0);
					} else {
						printf("Comanda gresita.\n");
						break;
					}
				} else if (i == sockTCP) {  // daca au venit date pe socketul pasiv
						// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
						// pe care serverul o accepta
						clilen = sizeof(cli_TCP_addr);
						newSockTCP = accept(sockTCP, (struct sockaddr *) &cli_TCP_addr, &clilen);
						DIE(newSockTCP < 0, "accept");

						// Dezactivez algoritmul lui Neagle
						int flag = 1;
						int disable = setsockopt(newSockTCP,
													IPPROTO_TCP,
													TCP_NODELAY,
													(char *)&(flag),
													sizeof(int));
						DIE(disable != 0, "disable");

						// se adauga noul socket intors de accept() la multimea
						// descriptorilor de citire
						FD_SET(newSockTCP, &read_fds);
						if (newSockTCP > fdmax) {
							fdmax = newSockTCP;
						}

						conexiune = 1;
						c.socket = newSockTCP;
						c.isConnected = 1;
					} else {
						// este un mesaj de la clientul TCP
						memset(bufferCLI, 0, 100);
						n = recv(i, bufferCLI, sizeof(bufferCLI), 0);
						int destSock = bufferCLI[0] - '0';  // conversie cod
						DIE(n < 0, "recv");

						// daca este socketul
						if(destSock <= fdmax) {
							send(destSock, bufferCLI + 2, BUFLEN, 0);
						}
					// mesaj de inchidere conexiune din partea TCP
					if (n == 0) {
						if (connect_same_id == 0) {
							printf("Client %s disconnected.\n", clients[i-5].ID_CLIENT);
						}

						connect_same_id = 0;
						clients[i - 5].isConnected = 0;
						close(i);

						// se scoate din multimea de citire socketul inchis
						FD_CLR(i, &read_fds);
					// mesaj de conectare din partea TCP
					} else if (conexiune == 1) {
						// verific daca se afla deja in vectorul de clienti
						int index = isAlreadyClient(clients, bufferCLI, clients.size());

						if (index == -1) {
							memcpy(c.ID_CLIENT, bufferCLI, sizeof(c.ID_CLIENT));
							memcpy(c.ip, inet_ntoa(cli_TCP_addr.sin_addr), sizeof(c.ip));

							c.port = ntohs(cli_TCP_addr.sin_port);
							c.socket = i;
							clients.push_back(c);

							printf("New client %s connected from %s:%d.\n",
									c.ID_CLIENT, c.ip, c.port);
								// daca este un fost client ce s-a reconectat
						} else if (clients[index].isConnected == 0) {
								clients[index].port = newSockTCP;
								clients[index].isConnected = 1;
								connect_same_id = 0;

								printf("New client %s connected from %s:%d.\n",
										clients[index].ID_CLIENT, clients[index].ip, clients[index].port);

								// trimit mesajele aflate in asteptare
								send_pending_messages(&clients[index]);
						} else {
							msg_srv* m;
							m = generate_message("You're already connected from another process.");
							int n = send(i, m, m->size + 3 * sizeof(int), 0);
							DIE(n < 0, "send");
							free(m);

							printf("Client %s already connected.\n", clients[index].ID_CLIENT);

							connect_same_id = 1;
						}
						conexiune = 0;
					} else {
						process_request(&topics, bufferCLI, i, &clients[i - 5]);
					}
				}
			}
		}
	}

	/*Inchidere socketi*/
	close(sockUDP);
	close(sockTCP);

	return 0;
}
