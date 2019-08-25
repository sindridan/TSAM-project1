//
// Simple server for TSAM-409 Assignment 1
//
// Compile: g++ -Wall -std=c++11 server.cpp 
//
// Command line: ./server 5000 
//
// Author: Jacky Mallett (jacky@ru.is)
//
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <string.h>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#endif

#define BACKLOG  5          // Allowed length of queue of waiting connections

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client
{
  public:
    int sock;              // socket of client connection
    std::string name;      // Limit length of name of client's user

    Client(int socket) : sock(socket){} 

    ~Client(){}            // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table, 
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client*> clients; // Lookup table for per Client information

// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

int open_socket(int portno)
{
   struct sockaddr_in sk_addr;   // address settings for bind()
   int sock;                     // socket opened for this port
   int set = 1;                  // for setsockopt

   // Create socket for connection. Note: OSX doesn´t support SOCK_NONBLOCK
   // so we have to use an fcntl (file control) command there instead. 

   #ifndef SOCK_NONBLOCK
      if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
      {
         perror("Failed to open socket");
         return(-1);
      }

      int flags = fcntl(sock, F_GETFL, 0);

      if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
      {
         perror("Failed to set O_NONBLOCK");
      }
   #else
      if((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK , IPPROTO_TCP)) < 0)
      {
         perror("Failed to open socket");
         return(-1);
      }
    #endif

   // Turn on SO_REUSEADDR to allow socket to be quickly reused after 
   // program exit.

   if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
   {
      perror("Failed to set SO_REUSEADDR:");
   }

   // Initialise memory
   memset(&sk_addr, 0, sizeof(sk_addr));

   // Set type of connection

   sk_addr.sin_family      = AF_INET;
   sk_addr.sin_addr.s_addr = INADDR_ANY;
   sk_addr.sin_port        = htons(portno);

   // Bind to socket to listen for connections from clients

   if(bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
   {
      perror("Failed to bind to socket:");
      return(-1);
   }
   else
   {
      return(sock);
   }
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{
     close(clientSocket);      

     // If this client's socket is maxfds then the next lowest
     // one has to be determined. Socket fd's can be reused by the Kernel,
     // so there aren't any nice ways to do this.

     if(*maxfds == clientSocket)
     {
        for(auto const& p : clients)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
     }

     // And remove from the list of open sockets.

     FD_CLR(clientSocket, openSockets);
}

// Process any message received from client on the server

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, 
                  char *buffer) 
{
  std::vector<std::string> tokens;     // List of tokens in command from client
  std::string token;                   // individual token being parsed

  // Split command from client into tokens for parsing
  std::stringstream stream(buffer);

  // By storing them as a vector - tokens[0] is first word in string
  while(stream >> token)
      tokens.push_back(token);

  // This assumes that the supplied command has no parameters
  if((tokens[0].compare("SYS") == 0) && (tokens.size() >= 2))
  {
    FILE *fp;
    char var[128];
    std::string result = "";

    //If system commands are more than 1, e.g. makedir <dir>
    std::string tokensTogether = "";
    for (int i = 1; i < tokens.size(); i++) {
        tokensTogether += tokens[i] + " ";
    }
    std::cout << tokensTogether << std::endl;
    fp = popen(tokensTogether.c_str(), "r");
    while(fgets(var, sizeof(var), fp) != NULL) {
        printf("%s", var);
        result += var; //Append output to be sent to client
    }
    pclose(fp);

    //In case that there is no output from the SYS command 
    if(result == "") {
        send(clientSocket, std::string(buffer).c_str(), std::string(buffer).size(), 0);
    } else {
        send(clientSocket, result.c_str(), result.size(), 0);
    }
  }
  else
  {
      std::cout << "Unknown command from client:" << buffer << std::endl;
  }
}

int main(int argc, char* argv[])
{
    bool finished;
    int listenSock;                 // Socket for connections to server
    int clientSock;                 // Socket of connecting client
    fd_set openSockets;             // Current open sockets 
    fd_set readSockets;             // Socket list for select()        
    fd_set exceptSockets;           // Exception socket list
    int maxfds;                     // Passed to select() as max fd in set
    struct sockaddr_in client;      // address of incoming client
    socklen_t clientLen;            // address length
    char buffer[1025];              // buffer for reading from clients
    std::vector<int> clientSocketsToClear; // List of closed sockets to remove


    if(argc != 2)
    {
        printf("Usage: server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to

    listenSock = open_socket(atoi(argv[1])); 

    printf("Listening on port: %d\n", atoi(argv[1]));
    printf("       (range of available ports on skel.ru.is is 4000-4100)\n");

    if(listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }
    else 
    // Add the listen socket to socket set
    {
        FD_SET(listenSock, &openSockets);
        maxfds = listenSock;
    }

    finished = false;

    while(!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // Accept  any new connections to the server
            if(FD_ISSET(listenSock, &readSockets))
            {
               clientSock = accept(listenSock, (struct sockaddr *)&client,
                                   &clientLen);

               FD_SET(clientSock, &openSockets);
               maxfds = std::max(maxfds, clientSock);

               clients[clientSock] = new Client(clientSock);
               n--;

               printf("Client connected on server\n");
            }
            // Check for commands from already connected clients
            while(n-- > 0)
            {
               for(auto const& pair : clients)
               {
                  Client *client = pair.second;

                  if(FD_ISSET(client->sock, &readSockets))
                  {
                      if(recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                      {
                          printf("Client closed connection: %d", client->sock);

                          closeClient(client->sock, &openSockets, &maxfds);
                          clientSocketsToClear.push_back(client->sock);

                      }
                      else
                      {
                        std::cout << buffer << std::endl;
                        clientCommand(client->sock, &openSockets, &maxfds, 
                                buffer);
                      }
                  }
               }

               // Remove client from the clients list. This has to be done 
               // out of the main loop, since we can't modify the iterator.
               for(auto const& i : clientSocketsToClear)
               {
                   clients.erase(i);
               }
            }
        }
    }
}
