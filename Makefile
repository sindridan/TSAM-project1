output: server-3.o p1client.o
	g++ -Wall -std=c++11 server-3.o -o server
	g++ -Wall -std=c++11 p1client.o -o client

server3.o: server-3.cpp
	g++ -c server-3.cpp

p1client.o: p1client.cpp
	g++ -c p1client.cpp

clean:
	rm *.o output