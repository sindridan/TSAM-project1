#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
using namespace std;
int main()
{
    int sockfd;
    int portno;
    char* srvr;
    int n;
    int iasd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];

    cout << "IP address: ";
    cin >> server;
    cout << "Port no: ";
    cin >> portno;
    server = gethostbyname(srvr);
    cout << "1" << endl;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    cout << "2" << endl;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    cout << "3" << endl;
    serv_addr.sin_family = AF_INET;
    cout << "4" << endl;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    cout << "5" << endl;
    serv_addr.sin_port = htons(portno);
  
    printf("Please enter the message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sockfd,buffer,strlen(buffer));
   
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    printf("%s\n",buffer);
    close(sockfd);
   
    return 0;
}
