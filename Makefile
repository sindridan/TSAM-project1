all: server client
server: server-3.cpp
	g++ -Wall -std=c++11 server-3.cpp -o server

client: p1client.cpp
	g++ -Wall -std=c++11 p1client.cpp -o client
