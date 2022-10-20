# Portul pe care asculta serverul (de completat)
PORT = 8082

# Adresa IP a serverului (de completat)
IP_SERVER = 127.0.0.1

all: server subscriber

server:
	g++ -Wall -Wextra server.cpp header.cpp -o server

subscriber:
	g++ -Wall -Wextra subscriber.cpp header.cpp -o subscriber

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
