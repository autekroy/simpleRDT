
/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include <stdio.h>
#include <netdb.h>      // define structures like hostent
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>   /* for the waitpid() system call */
#include <signal.h>     /* signal name macros, and the kill() prototype */
#include <sys/time.h>   // for setitimer
#include "packet.h"

#define SEND_BUFFER_SIZE    PACKET_SIZE             //in bytes
#define RECEIVE_BUFFER_SIZE PACKET_SIZE * 1024      //in bytes

void resend_window(int sig)
{
    //retransmit everything in my window
    printf("========= Timer expired on %d\n", base);
    state = RETRANSMIT;
}

int main(int argc, char *argv[])
{    
    int sockfd, portno;
    struct sockaddr_in snd_addr;
    char* file_ptr;
    int file_size;

    socklen_t snd_addr_len;
    struct hostent *server; //contains tons of information, including the server's IP address
    char *hostname, *filename;

    if (argc != 6)
        error("Usage: ./receiver <sender hostname> <sender port number> <filename> <loss rate> <corruption rate>\n");

    srand(time(NULL));

    //initializing sending and receiving state variables
    rdt_init();

    //retrieving user supplied arguments
    hostname = argv[1];
    portno = atoi(argv[2]);// port number
    filename = argv[3];
    loss_rate = atof(argv[4]);
    corrupt_rate = atof(argv[5]);
    win_size = 1;

    if(loss_rate < 0.0 || loss_rate > 1.0 || corrupt_rate < 0.0 || corrupt_rate > 1.0)
        error("ERROR impossible loss rate or corruption rate (should be between 0 and 1)");
    // printf("%s %d %s\n", hostname, portno, filename);

    // SOCK_DGRAM for UDP transfer
    // AF_INET force IPv4
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);   //create socket
    if (sockfd < 0) 
        error("ERROR opening socket");

    // Initialize server's address
    server = gethostbyname(hostname);
    if (server == NULL) 
        error("ERROR no such host");
    
    memset((char *) &snd_addr, 0, sizeof(snd_addr));  //reset memory
    snd_addr.sin_family = AF_INET; //initialize server's address
    bcopy((char *)server->h_addr, (char *)&snd_addr.sin_addr.s_addr, server->h_length);
    snd_addr.sin_port = htons(portno);
    // socket doesn't need to bind(), we just let OS assign a port number automatically

    // Build a request packet
    printf("Building request packet for file: %s\n", filename);
    out_pkt.type = DATA;
    out_pkt.seqnum = 1;
    out_pkt.ending_flag = 1;
    out_pkt.size = strlen(filename) + 1;// string end character
    strcpy(out_pkt.data, filename);

    // Send request packet
    print_packet(out_pkt, 0, 0);// send request pakcet, print data
    lastDataSeqNum = 1;
    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &snd_addr, sizeof(snd_addr)) == -1)
        error("ERROR on sending file request packet");
    nextSeqNum += 1;

    // char* sndBuffer[SEND_BUFFER_SIZE];
    // memset(sndBuffer, '\0', SEND_BUFFER_SIZE);
    
    unsigned int rcvBufferSize = RECEIVE_BUFFER_SIZE;
    unsigned int rcvBufferIndex = 0;
    char* rcvBuffer = (char*)malloc(sizeof(char) * RECEIVE_BUFFER_SIZE);

    //setup retransmisison timer
    time_out_val.it_value.tv_sec = TIMEOUT/1000;
    time_out_val.it_value.tv_usec = TIMEOUT*1000;   
    time_out_val.it_interval = time_out_val.it_value;

    //setup cancel timer
    time_out_val.it_value.tv_sec = 0;
    time_out_val.it_value.tv_usec = 0;   
    time_out_val.it_interval = time_out_val.it_value;

    // Enable alarm
    if (signal(SIGALRM, (void (*)(int)) resend_window) == SIG_ERR) {
        error("ERROR Unable to catch SIGALRM");
    }

    //setup select wait timer
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    //initialize state
    state = WAITING;

    //Declare readfds for select
    fd_set readfds;

    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        if(select(sockfd+1, &readfds, NULL, NULL, &tv)){
            //receive data
            if (recvfrom(sockfd, &in_pkt, sizeof(in_pkt), 0, (struct sockaddr*) &snd_addr, &snd_addr_len) == -1){
                error("ERROR on receiving file request packet");
            }
            print_packet(in_pkt, 1, 0); // receive request, print request

            //if received DATA
            if(in_pkt.type == DATA)
            {
                //if(debug) printf("[TRANSMIT]: received DATA\n");
                //grow receiver buffer as needed
                if(rcvBufferIndex + in_pkt.size >= rcvBufferSize)
                {
                    rcvBuffer = (char*)realloc(rcvBuffer, sizeof(char)*(rcvBufferSize+RECEIVE_BUFFER_SIZE));
                    rcvBufferSize += RECEIVE_BUFFER_SIZE;
                }
                int received = rdt_receive_data(rcvBuffer, &rcvBufferIndex, sockfd, &snd_addr);
                if(received == 0 && in_pkt.ending_flag == 1)
                {
                    state = PROCESSING;
                }
            }
            else if(in_pkt.type == ERR)
            {
                int received = rdt_receive_data(rcvBuffer, &rcvBufferIndex, sockfd, &snd_addr);
                if(received == 0)
                {
                    state = ERROR;
                }
            }
            //if received ACK
            else
            {
                //if(debug) printf("[TRANSMIT]: received ACK\n");
                rdt_receive_ack(sockfd, &snd_addr, file_ptr, file_size);
            }
        }
        else
        {
            if (state == RETRANSMIT)
            {
                if(debug) printf("[RETRANSMIT]: resend pkt from %d\n", base);

                nextSeqNum = base;
                rdt_send(sockfd, &snd_addr, file_ptr, file_size, DATA);

                if (signal(SIGALRM, (void (*)(int)) resend_window) == SIG_ERR) {
                    error("ERROR Unable to catch SIGALRM");
                }
                state = WAITING;
            }
            else if(state == ERROR)
            {
                // char error_msg[PACKET_SIZE+1];
                // memcpy(error_msg, in_pkt.data, in_pkt.size);
                // error_msg[in_pkt.size] = '\0';
                // printf("ERROR Received: %s", error_msg);

                // Error message is sent as a string
                printf("ERROR Received: %s\n\n", in_pkt.data);
                exit(0);
            }
            else if(state == PROCESSING)
            {
                // check if file exist
                if(debug) printf("[PROCESSING]: Process request: saving file, size: %d\n", rcvBufferIndex);

                char newfilename[100];
                memset(newfilename, '\0', sizeof(newfilename));
                strcat(newfilename, "received_");
                strcat(newfilename, filename);

                FILE *new_file = fopen(newfilename, "w");
                if (new_file == NULL)
                    error("Error opening file!\n");
                fwrite (rcvBuffer , sizeof(char), rcvBufferIndex, new_file);
                fclose (new_file);

                exit(0);
            }
            else //state == WAITING
            {
                if(debug) printf("[WAITING]: ...\n"); 
            }

        }
    }
    free(rcvBuffer);
     
    return 0; 
}

