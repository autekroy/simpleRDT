
/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <strings.h>

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in snd_addr;


    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    // SOCK_DGRAM for UDP transfer
    // AF_INET force IPv4
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);   //create socket
    if (sockfd < 0) error("ERROR opening socket");
    memset((char *) &snd_addr, 0, sizeof(serv_addr));  //reset memory

    //fill in address info for sender
    snd_addr.sin_family = AF_INET;
    snd_addr.sin_addr.s_addr = htonl(atoi(argv[1]));;
    snd_addr.sin_port = htons(atoi(argv[2]));

    
     
    return 0; 
}

