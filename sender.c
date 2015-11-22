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
    fprintf(stderr, "%s", msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, win_size;
    struct sockaddr_in snd_addr, rcv_addr;
    char buff[256];
    socklen_t rcv_len;

    if (argc != 3)
       error("Usage: ./sender <portnumber> <window size>\n");

    // use SOCK_DGRAM for UDP transfer
    // AF_INET force IPv4
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);	//create socket
    if (sockfd < 0) 
       error("ERROR opening socket");

    portno = atoi(argv[1]);
    win_size = atoi(argv[2]);

    memset((char *) &snd_addr, 0, sizeof(snd_addr));	//reset memory
    //fill in address info
    snd_addr.sin_family = AF_INET;
    snd_addr.sin_addr.s_addr = INADDR_ANY;
    snd_addr.sin_port = htons(portno);
    
    // bind the port number to this socket     
    if (bind(sockfd, (struct sockaddr *) &snd_addr,
              sizeof(snd_addr)) < 0) 
              error("ERROR on binding");
    
    // not sure if we need it or not
    listen(sockfd, 5); 

    rcv_len = sizeof(rcv_addr);

    // keep receiving file request
    while(1){
        memset(buff, '\0', 256);
        // receive file request packet
        printf("receive file request packet\n");
        if (recvfrom(sockfd, buff, 256, 0, (struct sockaddr*) &rcv_addr, &rcv_len) == -1){
            error("ERROR on receiving file request packet");
            continue;
        }
        printf("%s\n", buff);

        // check file exist

        // send ACK about file infor (not exist or total length)

        // get ACK for receiver know the file infor

        // send file

    }

    return 0; 
}

