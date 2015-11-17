/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */


void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd;
     socklen_t clilen;
     struct sockaddr_in snd_addr, rcv_addr;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     // use SOCK_DGRAM for UDP transfer
     // AF_INET force IPv4
     sockfd = socket(AF_INET, SOCK_DGRAM, 0);	//create socket
     if (sockfd < 0) 
        error("ERROR opening socket");
     memset((char *) &snd_addr, 0, sizeof(serv_addr));	//reset memory

     //fill in address info
     snd_addr.sin_family = AF_INET;
     snd_addr.sin_addr.s_addr = INADDR_ANY;
     snd_addr.sin_port = htons(atoi(argv[1]));
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     
     
         
     return 0; 
}

