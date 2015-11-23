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
#include <signal.h>	    /* signal name macros, and the kill() prototype */
#include <sys/time.h>   // for setitimer

#include "packet.h"

#define WAITING     0
#define PROCESSING  1

void error(char *msg)
{
    fprintf(stderr, "%s", msg);
    exit(0);
}



int main(int argc, char *argv[])
{
    int sockfd, portno, win_size;
    struct sockaddr_in snd_addr, rcv_addr;
    //char buff[256];
    socklen_t rcv_len;
    packet_t in_pkt, out_pkt;
    char buffer[PACKET_SIZE];
    char* allData;

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
    //listen(sockfd, 5); 

    /* version 1
    rcv_len = sizeof(rcv_addr);

    // keep receiving file request
    while(1){
        memset(buff, '\0', 256);
        // receive file request packet
        printf("receive file request packet\n");
        // if (recvfrom(sockfd, buff, 256, 0, (struct sockaddr*) &rcv_addr, &rcv_len) == -1){
        //     error("ERROR on receiving file request packet");
        //     continue;
        // }
        // printf("%s\n", buff);
        
        if (recvfrom(sockfd, &in_pkt, sizeof(in_pkt), 0, (struct sockaddr*) &rcv_addr, &rcv_len) == -1){
            error("ERROR on receiving file request packet");
            continue;
        }
        print_packet(in_pkt, 1, 1); // receive request, print request

        

        // check file exist

        // send ACK about file infor (not exist or total length)

        // get ACK for receiver know the file infor

        // send file

    }
    */

    //initializing readfds for select
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    char state = WAITING;

    //initializing sending and receiving state variables
    expSeqNum = 1;
    base = 1;
    nextSeqNum = base;

    char* sndBuffer[SEND_BUFFER_SIZE];
    memset(sndBuffer, '\0', SEND_BUFFER_SIZE);
    
    //supports up to 4GB-1 size of file
    unsigned int rcvBufferSize = RECEIVE_BUFFER_SIZE;
    unsigned int rcvBufferIndex = 0;
    char* rcvBuffer = (char*)malloc(sizeof(RECEIVE_BUFFER_SIZE));

    //setup timer
    struct itimerval time_out_val;    /* for setting itimer */

    if (signal(SIGALRM, (void (*)(int)) alarm_handler) == SIG_ERR) {
        perror("Unable to catch SIGALRM");
        exit(1);
    }
    time_out_val.it_value.tv_sec = TIMEOUT/1000;
    time_out_val.it_value.tv_usec = (TIMEOUT*1000) % 1000000;   
    time_out_val.it_interval = time_out_val.it_value;


    while(1)
    {
        if(select(sockfd+1, &readfds, NULL, NULL, 1))
        {
            //receive data
            if (recvfrom(sockfd, &in_pkt, sizeof(in_pkt), 0, (struct sockaddr*) &rcv_addr, &rcv_len) == -1){
                error("ERROR on receiving file request packet");
            }
            print_packet(in_pkt, 1, 0); // receive request, print request

            //if received DATA
            if(in_pkt.type == DATA)
            {
                //store received packet
                if (in_pkt.seqnum == expSeqNum)
                {
                    if (debug) printf("Packet seqnum %d stored\t", in_pkt.seqnum);
                    if(rcvBufferIndex + in_pkt.size >= rcvBufferSize)
                    {
                        rcvBuffer = (char*)realloc(rcvBuffer, rcvBufferSize+RECEIVE_BUFFER_SIZE);
                        rcvBufferSize += RECEIVE_BUFFER_SIZE;
                    }
                    memcpy(rcvBuffer+rcvBufferIndex, &in_pkt.data, in_pkt.size);
                    rcvBufferIndex += in_pkt.size; 
                    expSeqNum++;

                    //transition state if completed receiving
                    if(in_pkt.ending_flag = 1)
                    {
                        if (debug) printf("(last pkt)");
                        state = PROCESSING;
                    }
                    if (debug) printf("\n");
                }
                else
                {
                    if (debug) printf("***** Packet seqnum %d dropped\n", in_pkt.seqnum);
                }

                //Enhancement: create a ACK specific package to reduce ACK size
                out_pkt.type = ACK;
                out_pkt.seqnum = expSeqNum - 1;
                out_pkt.size = 0;

                print_packet(out_pkt, 0, 0);// send request pakcet, print data
                if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &rcv_addr, rcv_len) == -1)
                    error("ERROR on sending file request packet\n");     

            }
            //if received ACK
            else
            {
                //receive acknowledgement
                if (in_pkt.seqnum >= base)
                {
                    //sliding sending window
                    base = ack_num + 1;
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
                    /*
                    if(nextSeqNum == base) //when sending first packet
                        setitimer(TIMEOUT)
                    */
                    out_pkt.type = DATA;
                    out_pkt.seqnum = nextSeqNum;
                    out_pkt.size = get_file_segment(nextSeqNum, file_ptr, &out_pkt.data, PACKET_SIZE);
                    
                    // check if it's last packet
                    if(nextSeqNum == lastDataSeqNum)
                        out_pkt.ending_flag = 1;

                    print_packet(out_pkt, 0, 0);// send request pakcet, print data
                    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &rcv_addr, rcv_len) == -1)
                        error("ERROR on sending file request packet\n");  

                    nextSeqNum++
                }
            }
        }
        else
        {
            if(state == PROCESSING)
            {
                //if file exist, open file
                //if file doens't exist, save error message into buffer

                lastDataSeqNum =  file_size/PACKET_SIZE;
                if(file_size % PACKET_SIZE != 0)
                    lastDataSeqNum += 1;
                

            }

        }
    }


    return 0; 
}

