
/*
 A simple client in the internet domain using TCP
 Usage: ./client hostname port (./client 192.168.0.151 10000)
 */
#include <stdio.h>
#include <netdb.h>      // define structures like hostent
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>   /* for the waitpid() system call */
#include <signal.h>     /* signal name macros, and the kill() prototype */
#include <sys/time.h>   // for setitimer
#include "packet.h"

#define WAITING     0
#define PROCESSING  1

void error(char *msg)
{
    fprintf(stderr, "%s", msg);
    exit(0);
}

void resend_window(int sig)
{
    //retransmit everything in my window
    printf("Timer expired");
}

int simulate(double probability) {
    double random_value = (double) rand() / (double) RAND_MAX;
    if (random_value <  probability)
        return 1;
    return 0;
}


int main(int argc, char *argv[])
{
    double loss_rate, corrupt_rate;
    int sockfd, portno;
    struct sockaddr_in snd_addr;
    struct hostent *server; //contains tons of information, including the server's IP address
    char *hostname, *filename;
    socklen_t snd_len;
    packet_t out_pkt, in_pkt;

    //file variables
    char* file_ptr;
    int file_size;

    if (argc != 6)
        error("Usage: ./receiver <sender hostname> <sender portnumber> <filenam>\n");

    srand(time(NULL));

    int win_size = 1, lastDataSeqNum = 1;
    //initializing readfds for select
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    int state = WAITING;

    //initializing sending and receiving state variables
    expSeqNum = 1;
    base = 1;
    nextSeqNum = base;

    hostname = argv[1];
    portno = atoi(argv[2]);// port number
    filename = argv[3];
    loss_rate = atof(argv[4]);
    corrupt_rate = atof(argv[5]);

    if(loss_rate < 0.0 || loss_rate > 1.0 || corrupt_rate < 0.0 || corrupt_rate > 1.0)
        error("impossible loss rate or corruption rate (should be between 0 and 1)");
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
    
    memset((char *) &snd_addr, 0, sizeof(snd_addr));  //reset memory
    snd_len = sizeof(snd_addr);
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

    // request packet should look like this
    print_packet(out_pkt, 0, 0);// send request pakcet, print data
    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &snd_addr, snd_len) == -1)
        error("ERROR on sending file request packet\n");
    nextSeqNum += 1;

    char* sndBuffer[SEND_BUFFER_SIZE];
    memset(sndBuffer, '\0', SEND_BUFFER_SIZE);
    
    unsigned int rcvBufferSize = RECEIVE_BUFFER_SIZE;
    unsigned int rcvBufferIndex = 0;
    char* rcvBuffer = (char*)malloc(sizeof(char) * RECEIVE_BUFFER_SIZE);

    //setup timer
    struct itimerval time_out_val;    /* for setting itimer */

    if (signal(SIGALRM, (void (*)(int)) resend_window) == SIG_ERR) {
        perror("Unable to catch SIGALRM");
        exit(1);
    }
    time_out_val.it_value.tv_sec = TIMEOUT/1000;
    time_out_val.it_value.tv_usec = (TIMEOUT*1000) % 1000000;   
    time_out_val.it_interval = time_out_val.it_value;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        if(select(sockfd+1, &readfds, NULL, NULL, &tv) < 0){
            printf("select error\n");
        } 
        else if (FD_ISSET(sockfd, &readfds)) 
        {
            
            //receive data
            if (recvfrom(sockfd, &in_pkt, sizeof(in_pkt), 0, (struct sockaddr*) &snd_addr, &snd_len) == -1){
                error("ERROR on receiving file request packet");
            }
            print_packet(in_pkt, 1, 0); // receive request, print request

            //if received DATA
            if(in_pkt.type == DATA)
            {

                // lose this DATA (didn't receive)
                if(simulate(loss_rate)){
                    printf("Simulate didn't get DATA %d\n", in_pkt.seqnum);
                    continue;
                }           
                else if (simulate(corrupt_rate)) {
                    printf("Simulate DATA %d is corrupted\n", in_pkt.seqnum);
                    // resend the expected ACK
                    out_pkt.type = ACK;
                    out_pkt.seqnum = expSeqNum - 1;
                    out_pkt.size = 0;
                    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &snd_addr, snd_len) == -1)
                        error("ERROR on sending file request packet\n");     
                    print_packet(out_pkt, 0, 0);// send request pakcet, print data
                    continue;
                }

                //store received packet
                if (in_pkt.seqnum == expSeqNum)
                {
                    if (debug) printf("Packet seqnum %d stored. Pkt size: %d, bufferIndex: %d, buffersize: %d\t", in_pkt.seqnum, in_pkt.size, rcvBufferIndex, rcvBufferSize);
                    if(rcvBufferIndex + in_pkt.size >= rcvBufferSize)
                    {
                        rcvBuffer = (char*)realloc(rcvBuffer, sizeof(char) * (rcvBufferSize+RECEIVE_BUFFER_SIZE));
                        rcvBufferSize += RECEIVE_BUFFER_SIZE;
                    }
                    memcpy(rcvBuffer+rcvBufferIndex, &in_pkt.data, in_pkt.size);
                    rcvBufferIndex += in_pkt.size; 
                    expSeqNum++;

                    //transition state if completed receiving
                    if(in_pkt.ending_flag == 1)
                    {
                        if (debug) printf("(last pkt)");
                        state = PROCESSING;
                    }
                    if (debug) printf("\n");
                }
                else
                {
                    if (debug) printf("====== Drop DATA %d\n", in_pkt.seqnum);
                }

                //Enhancement: create a ACK specific package to reduce ACK size
                out_pkt.type = ACK;
                out_pkt.seqnum = expSeqNum - 1;
                out_pkt.size = 0;

                if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &snd_addr, snd_len) == -1)
                    error("ERROR on sending file request packet\n");   
                print_packet(out_pkt, 0, 0);// send request pakcet, print data  

            }
            //if received ACK
            else
            {
                //receive acknowledgement
                if (in_pkt.seqnum >= base)
                {
                    //sliding sending window
                    base = in_pkt.seqnum + 1; //in_pkt.seqnum -> ack num
                    /*
                    if(base == nextseqnum)
                        setitimer(0);
                    else 
                        setitimer(TIMEOUT);
                    */
                }
                //add print statement

                //send data while there are remaining data in the window
                while (nextSeqNum < base + win_size && nextSeqNum <= lastDataSeqNum)
                {
                    if (debug) printf("has remaining data\n");
                    
                    if(nextSeqNum == base){ //when sending first packet
                        // setitimer(TIMEOUT)
                    }
                    
                    out_pkt.type = DATA;
                    out_pkt.seqnum = nextSeqNum;
                    out_pkt.size = get_file_segment(nextSeqNum, file_ptr, out_pkt.data, PACKET_SIZE, file_size);
                    
                    // check if it's last packet
                    if(nextSeqNum == lastDataSeqNum)
                        out_pkt.ending_flag = 1;

                    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &snd_addr, snd_len) == -1)
                        error("ERROR on sending file request packet\n"); 
                    print_packet(out_pkt, 0, 0);// send request pakcet, print data 

                    nextSeqNum++;
                }
            }
        }
        else
        {
            if(state == WAITING)
            {
                if(debug) printf("waiting %d\n", state);                
            }
            
            if(state == PROCESSING)
            {
                // check if file exist
                if(debug) printf("Process request: saving file, size: %d\n", rcvBufferIndex);

                char newfilename[100];
                memset(newfilename, '\0', sizeof(newfilename));
                strcat(newfilename, "received_");
                strcat(newfilename, filename);

                FILE *new_file = fopen(newfilename, "w");
                if (new_file == NULL)
                    error("Error opening file!\n");
                fwrite (rcvBuffer , sizeof(char), rcvBufferIndex, new_file);
                fclose (new_file);

                state = WAITING;
            }

        }
    }
    free(rcvBuffer);
     
    return 0; 
}

