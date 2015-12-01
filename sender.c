/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <sys/time.h>        /* for setitimer */
#include <unistd.h>     /* for pause */
#include <signal.h>     /* for signal */
#include "packet.h"

#define SEND_BUFFER_SIZE        PACKET_SIZE     //in bytes
#define RECEIVE_BUFFER_SIZE     PACKET_SIZE     //in bytes

void resend_window(int sig)
{
    //retransmit everything in my window
    state = RETRANSMIT;
    printf("========= Timer expired on seqnum = %d state = %d\n", base, state);
    // if (signal(SIGALRM, (void (*)(int)) resend_window) == SIG_ERR) {
    //     error("ERROR Unable to catch SIGALRM");
    // }
}

int main(int argc, char *argv[])
{
    int file_found = 0;

    int sockfd, portno;
    struct sockaddr_in snd_addr, rcv_addr;
    char* file_ptr;
    int file_size;

    socklen_t rcv_addr_len;

    if (argc != 5)
       error("Usage: ./sender <portnumber> <window size> <loss rate> <corrupt rate>\n");

    srand(time(NULL));

    // use SOCK_DGRAM for UDP transfer
    // AF_INET force IPv4
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);	//create socket
    if (sockfd < 0) 
       error("ERROR opening socket");

    portno = atoi(argv[1]);
    win_size = atoi(argv[2]);
    loss_rate = atof(argv[3]);
    corrupt_rate = atof(argv[4]);

    if(loss_rate < 0.0 || loss_rate > 1.0 || corrupt_rate < 0.0 || corrupt_rate > 1.0)
        error("ERROR impossible loss rate or corruption rate (should be between 0 and 1)");

    memset((char *) &snd_addr, 0, sizeof(snd_addr));	//reset memory
    
    //bind the port number to this socket  
    snd_addr.sin_family = AF_INET;
    snd_addr.sin_addr.s_addr = INADDR_ANY;
    snd_addr.sin_port = htons(portno);   
    if (bind(sockfd, (struct sockaddr *) &snd_addr, sizeof(snd_addr)) < 0) 
        error("ERROR on binding");

    //initializing readfds for select
    fd_set readfds;

    //initializing sending and receiving state variables
    state = WAITING;
    rdt_init();

    // char* sndBuffer[SEND_BUFFER_SIZE];
    // memset(sndBuffer, '\0', SEND_BUFFER_SIZE);
    
    //supports up to 4GB-1 size of file
    unsigned int rcvBufferSize = RECEIVE_BUFFER_SIZE;
    unsigned int rcvBufferIndex = 0;
    char* rcvBuffer = (char*)malloc(sizeof(char) * RECEIVE_BUFFER_SIZE);

    //setup retransmisison timer
    time_out_val.it_value.tv_sec = TIMEOUT/1000;
    time_out_val.it_value.tv_usec = (TIMEOUT*1000)%1000000;   
    time_out_val.it_interval.tv_sec = 0;
    time_out_val.it_interval.tv_usec = 0;

    //setup cancel timer
    time_out_cancel.it_value.tv_sec = 0;
    time_out_cancel.it_value.tv_usec = 0;   
    time_out_cancel.it_interval = time_out_cancel.it_value;

    // bind SIGALRM with callback function
    if (signal(SIGALRM, (void (*)(int)) resend_window) == SIG_ERR) {
        error("ERROR Unable to catch SIGALRM");
    }

    //setup select wait timer
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        if(select(sockfd+1, &readfds, NULL, NULL, &tv))
        {
            //receive data
            if (recvfrom(sockfd, &in_pkt, sizeof(in_pkt), 0, (struct sockaddr*) &rcv_addr, &rcv_addr_len) == -1){
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
                int received = rdt_receive_data(rcvBuffer, &rcvBufferIndex, sockfd, &rcv_addr);
                if(received == 0 && in_pkt.ending_flag == 1)
                {
                    state = PROCESSING;
                }
            }
            else //if received ACK
            {
                //if(debug) printf("[TRANSMIT]: received ACK\n");
                int received = rdt_receive_ack(sockfd, &rcv_addr, file_ptr, file_size);
                if(received == 0 && in_pkt.seqnum == lastDataSeqNum){
                    if(debug) printf("File transfer completed\n");
                    //reinitialize for next transmission
                    //rdt_init();
                    //exit(0);
                    state = WAITING;
                }

            }
        }
        else
        {
            if (state == RETRANSMIT)
            {
                if(debug) printf("[RETRANSMIT]: resend pkt from seqnum %d\n", base);
                if (file_found == 1)
                    rdt_retransmit(sockfd, &rcv_addr, file_ptr, file_size, DATA);
                else
                    rdt_retransmit(sockfd, &rcv_addr, file_ptr, file_size, ERR);
                state = TRANSMITTING;
            }
            else if(state == PROCESSING)
            {
                if(debug) printf("[PROCESSING]: Process request: %s\n", rcvBuffer);

                //Open file and read
                //file name is sent as a string with ending \0
                FILE* file = fopen(rcvBuffer , "r");
                //If the file does not exist
                packet_type_t packet_type;
                if(file == NULL){
                    //error("ERROR 404: File not found\n");
                    packet_type = ERR;
                    file_ptr = "File requested not found";
                    file_size = strlen(file_ptr)+1;
                    
                }
                else{
                    //Obtain file size:
                    //fseek: Reposition stream position indicator
                    //SEEK_END: End of file
                    file_found = 1;

                    packet_type = DATA;
                    fseek(file , 0 , SEEK_END);
                    file_size = ftell(file); //Get current position in stream
                    rewind(file); //Set position of stream to the beginning

                    // allocate memory to contain the whole file:
                    file_ptr = (char*) malloc(sizeof(char)*file_size);
                    if(file_ptr == NULL) error("ERROR Memory error");

                    // copy the file into the buffer:
                    size_t readResult;
                    readResult = fread(file_ptr,1,file_size,file);
                    if(readResult != file_size) error("ERROR Reading error");

                }

                // take the ceiling
                lastDataSeqNum = file_size/PACKET_SIZE;
                if(file_size % PACKET_SIZE != 0)
                    lastDataSeqNum += 1;
                
                if(debug) printf("[PROCESSING]: file_size: %d\tlastDataSeqNum: %d\n", file_size, lastDataSeqNum);

                rdt_send(sockfd, &rcv_addr, file_ptr, file_size, packet_type);

                //reset the receiver buffer
                free(rcvBuffer);
                rcvBufferSize = RECEIVE_BUFFER_SIZE;
                rcvBufferIndex = 0;
                rcvBuffer = (char*)malloc(sizeof(char) * RECEIVE_BUFFER_SIZE);

                state = TRANSMITTING;
            }
            else if(state == WAITING)
            {
                sleep(1);
                if(debug) printf("[WAITING]: ...\n");           
            }
        }
    }
    

    return 0; 
}

