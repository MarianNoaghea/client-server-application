#pragma once
#include "helpers.h"
#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string>

// structura in care se stocheaza
// un mesaj UDP
typedef struct msg{
	char topicName[50];
	char type;
	char string_value[1500];
}__attribute__((packed)) msg;

// structura trimisa de server
// mai departe
typedef struct msg_srv {
	int size;
	int ip;
	int port;
	msg mesaj;
}__attribute__((packed)) msg_srv;

struct topic;

typedef struct client {
	char ID_CLIENT[10];
	char ip[15];
	int port;
	int socket;
	int isConnected;
	map<string, int> SFMap;
	vector<msg_srv*> pending_messages;
}client;

typedef struct topic {
	char name[50];
	vector<client*> clients;
}topic;

// ---- functii pentru server-----
void send_pending_messages(client* c);
void send_to_subscribers(vector<topic*> topics, msg* mesaj, int ip, int port, int size);
int isAlreadyClient(vector<client> clients, char* clientName, int n);
void process_request(vector<topic*> topics[], char* bufferCLI,int socket, client* client);
msg_srv* generate_message(const char* string);
int already_subscribed(topic topic, client client);
void unsubscribe(topic* topic, client client);
int topic_exists(vector<topic*> topics[], char* topicName);

// ------ functii pentru subscriber----
long convert_INT(char* buffer);
float convert_SHORT_REAL(char* buffer);
float convert_FLOAT(char* buffer, int* power);
void process_payload(msg_srv* m);
