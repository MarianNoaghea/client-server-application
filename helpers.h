#ifndef _HELPERS_H
#define _HELPERS_H 1
/* Dimensiunea maxima a calupului de date */
#define BUFLEN 1551
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare

#include <bits/stdc++.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <map>

using namespace std;

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)
#endif
