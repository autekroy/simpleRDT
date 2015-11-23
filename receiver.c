
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
#include "packet.h"

void error(char *msg)
{
    fprintf(stderr, "%s", msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server; //contains tons of information, including the server's IP address
    char *hostname, *filename;
    socklen_t serv_len;
    packet_t req_pkt;

    if (argc != 4)
        error("Usage: ./receiver <sender hostname> <sender portnumber> <filenam>\n");

    hostname = argv[1];
    portno = atoi(argv[2]);// port number
    filename = argv[3];
    // printf("%s %d %s\n", hostname, portno, filename);

    // SOCK_DGRAM for UDP transfer
    // AF_INET force IPv4
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);   //create socket
    if (sockfd < 0) 
        error("ERROR opening socket");

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    memset((char *) &serv_addr, 0, sizeof(serv_addr));  //reset memory
    serv_len = sizeof(serv_addr);
    serv_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // socket doesn't need to bind(), we just let OS assign a port number automatically


    // Build a request packet
    printf("Building request packet for file: %s\n", filename);
    req_pkt.type = DATA;
    req_pkt.seqnum = 0;
    req_pkt.size = strlen(filename) + 1;// string end character
    strcpy(req_pkt.data, filename);

    // request_packet

    // Send request packet
    // printf("Send request packet\n");
    // if (sendto(sockfd, filename, strlen(filename), 0, (struct sockaddr*) &serv_addr, serv_len) == -1)
        // error("ERROR on sending file request packet\n");

    // request packet should look like this
    print_packet(req_pkt, 0, 0);// send request pakcet, print data
    if (sendto(sockfd, &req_pkt, sizeof(req_pkt), 0, (struct sockaddr*) &serv_addr, serv_len) == -1)
        error("ERROR on sending file request packet\n");

    // get Ack for this request

    // file not exist, error
    // file exist: send ACK

    // receiving files

    // maybe send FIN flag packet
     
    return 0; 
}

