#include "header.h"
#include "helpers.h"

// --------functii pentru server-------------

// functie ce verifica daca un topic exista deja
int topic_exists(vector<topic*> topics[], char* topicName) {
	for (size_t i = 0; i < topics->size(); i++) {
		 if (strcmp((*topics)[i]->name, topicName) == 0) {
		 	return i;
		}
	}
	return -1;
}

void unsubscribe(topic* topic, client client) {
	for (size_t i = 0; i < topic->clients.size(); i++) {
		if (strcmp(topic->clients[i]->ID_CLIENT, client.ID_CLIENT) == 0) {
			topic->clients.erase(topic->clients.begin() + i);
			return;
		}
	}
}

int already_subscribed(topic topic, client client) {
	for (size_t i = 0; i < topic.clients.size(); i++) {
		if (strcmp(topic.clients[i]->ID_CLIENT, client.ID_CLIENT) == 0) {
			return 1;
		}
	}
	return 0;
}

msg_srv* generate_message(const char* string) {
	msg mesaj;
	strcpy(mesaj.topicName, string);
	msg_srv* m =(msg_srv*) calloc(1, sizeof(msg_srv));
	memcpy(&m->mesaj, &mesaj, sizeof(msg));
	m->size = strlen(string);

	return m;
}

// aceasta functie aboneaza/dezaboneaza
// clientii la topicul corespunzator din vectorul de topics
void process_request(vector<topic*> topics[], char* bufferCLI, int socket, client* client) {
	int SF;
	char topicName[50];
	msg_srv* m;
	int n;

	if (strncmp(bufferCLI, "subscribe", 9) == 0) {
		sscanf(bufferCLI, "%*s%s%d", topicName, &SF);
		// adaug in Mapul <topicName, SF> pentru a tine cont
		// la ce topicuri ale clientului SF este 1

		// verific daca exista deja
		int topic_index = topic_exists(topics, topicName);

		if (topic_index == -1) {
			client->SFMap[topicName] = SF;
			topic* t = (topic*)calloc(1, sizeof(topic));
			strcpy(t->name, topicName);
			t->clients.push_back(client);
			topics->push_back(t);
		} else if (already_subscribed(*(*topics)[topic_index], *client)) {
				// daca e o subscriptie cu acelasi SF
				if (client->SFMap[topicName] == SF) {
					m = generate_message("You're already subscribed to this topic.");
					n = send(socket, m, m->size + 3 * sizeof(int), 0);
					DIE(n < 0, "send");
					free(m);
					// daca exista topicul dar cererea este pentru un SF diferit de cel anterior
				} else {
					client->SFMap[topicName] = SF;
					m = generate_message("Subscribed to topic.");
					n = send(socket, m, m->size + 3 * sizeof(int), 0);
					DIE(n < 0, "send");
					free(m);
				}

			return;
		 } else {
			 client->SFMap[topicName] = SF;
			 (*topics)[topic_index]->clients.push_back(client);
		 	}
		m = generate_message("Subscribed to topic.");
		n = send(socket, m, m->size + 3 * sizeof(int), 0);
		DIE(n < 0, "send");
		free(m);


	} else if (strncmp(bufferCLI, "unsubscribe", 11) == 0) {
		sscanf(bufferCLI, "%*s%s", topicName);
		int topic_index = topic_exists(topics, topicName);

		if (topic_index == -1) {
			m = generate_message("Topic doesn't exist.");
			n = send(socket, m, m->size + 3 * sizeof(int), 0);
			DIE(n < 0, "send");
			free(m);
			return;
		} else {
			unsubscribe((*topics)[topic_index], *client);
		}

		m = generate_message("Unsubscribed from topic.");
		n = send(socket, m, m->size + 3 * sizeof(int), 0);
		DIE(n < 0, "send");
		free(m);
	}
}


int isAlreadyClient(vector<client> clients, char* clientName, int n) {
	for (int i = 0; i < n; i++) {
		if (strcmp(clients[i].ID_CLIENT, clientName) == 0) {
			return i;
		}
	}
	return -1;
}

void send_to_subscribers(vector<topic*> topics, msg* mesaj, int ip, int port, int size) {
	// trimit clientilor TCP o noua structura care va contine size, port, ip si
	// structura initiala
	msg_srv* m = (msg_srv*) calloc(1, sizeof(msg_srv));
	memcpy(&m->mesaj, mesaj, size);
	m->size = size;
	m->ip = ip;
	m->port = port;

	for(auto t : topics) {
		if (strcmp(t->name, mesaj->topicName) == 0) {
			for (auto c : t->clients) {
				if (c->isConnected) {
					int n = send(c->socket, m, m->size + 3 * sizeof(int) , 0);
					DIE(n < 0, "send");
				} else {
					// caut in Mapul<topicName,SF> daca SF = 1;
					if (c->SFMap[t->name]) {
						c->pending_messages.push_back(m);
					}
				}
			}
		}
	}
}

void send_pending_messages(client* c) {
	// daca nu exista mesaje nu se trimite nimic
	for (auto msg : c->pending_messages) {
		int n = send(c->socket, msg, msg->size + 3 * sizeof(int), 0);
		DIE(n < 0, "send");
		free(msg);
	}
	c->pending_messages.clear();
}

// --------------------functii pentru subscriber--------------------
long convert_INT(char* buffer) {
	int bitSemn = buffer[0];
	long value = ntohl(*(uint32_t*)(buffer + 1));

	if (bitSemn == 1) {
		value *= -1;
	}

	return value;
}

float convert_SHORT_REAL(char* buffer) {
	float value = (float)ntohs(*(uint16_t*)buffer) / 100;

	return value;
}

float convert_FLOAT(char* buffer, int* power) {
	int bitSemn = buffer[0];
	float value = ntohl(*(uint32_t*)(buffer + 1));
	*power = (int) buffer[5];

	for (int i = 0; i < *power; i++) {
		value /= (double)10;
	}

	if (bitSemn == 1) {
		value *= -1;
	}

	return value;
}

void process_payload(msg_srv* m) {
	char ip[16];
	inet_ntop(AF_INET, &m->ip, ip, INET_ADDRSTRLEN);


	if (m->mesaj.type == 0) {
		printf("%s:%d - %s - INT - %ld\n",
				ip , ntohs(m->port), m->mesaj.topicName, convert_INT(m->mesaj.string_value));
	} else if (m->mesaj.type == 1) {
		printf("%s:%d - %s - SHORT_REAL - %.2f\n",
				ip , ntohs(m->port), m->mesaj.topicName, convert_SHORT_REAL(m->mesaj.string_value));
	}  else if (m->mesaj.type == 2) {
		int power = 0;
		float number = convert_FLOAT(m->mesaj.string_value, &power);
		printf("%s:%d - %s - FLOAT - %.*f\n",
				ip , ntohs(m->port), m->mesaj.topicName, power, number);
	}   else if (m->mesaj.type == 3) {
		printf("%s:%d - %s - STRING - %s\n",
				ip , ntohs(m->port), m->mesaj.topicName, m->mesaj.string_value);
	}
}
