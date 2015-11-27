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

int resend;
void resend_window(int sig)
{
    //retransmit everything in my window
    printf("========= Timer expired on %d\n", base);
    nextSeqNum = base;
    resend = 1;
}

int simulate(double probability) {
    double random_value = (double) rand() / (double) RAND_MAX;
    if (random_value <  probability)
        return 1;
    return 0;
}


int main(int argc, char *argv[])
{
    int sockfd, portno, win_size;
    double loss_rate, corrupt_rate;
    struct sockaddr_in snd_addr, rcv_addr;
    //char buff[256];
    socklen_t rcv_len;
    packet_t in_pkt, out_pkt;
    char buffer[PACKET_SIZE];
    char* allData;

    //file variables
    char* file_ptr;
    int file_size;
    resend = 0;

    if (argc != 5)
       error("Usage: ./sender <portnumber> <window size>\n");

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
        error("impossible loss rate or corruption rate (should be between 0 and 1)");

    memset((char *) &snd_addr, 0, sizeof(snd_addr));	//reset memory
    //fill in address info
    snd_addr.sin_family = AF_INET;
    snd_addr.sin_addr.s_addr = INADDR_ANY;
    snd_addr.sin_port = htons(portno);
    
    // bind the port number to this socket     
    if (bind(sockfd, (struct sockaddr *) &snd_addr, sizeof(snd_addr)) < 0) 
              error("ERROR on binding");

    //initializing readfds for select
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    int state = WAITING;

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

        if(select(sockfd+1, &readfds, NULL, NULL, &tv) < 0){// select failure
            printf("select error\n");
            if(resend == 1){
                printf("0 ========= Timer expired, resend pkt from %d\n", base);
                //send data while there are remaining data in the window
                while (nextSeqNum < base + win_size && nextSeqNum <= lastDataSeqNum)
                {
                    if(nextSeqNum == base){ //when sending first packet

                        // setting timer
                        time_out_val.it_value.tv_sec = TIMEOUT/1000;
                        time_out_val.it_value.tv_usec = (TIMEOUT*1000) % 1000000;   
                        time_out_val.it_interval = time_out_val.it_value;
                        if (setitimer(ITIMER_REAL, &time_out_val, NULL) == -1) {
                           perror("error calling setitimer()");
                           exit(1);
                        }
                        else{
                            resend = 0;
                            printf("Reset timer because resending from %d\n", base);
                        }
                    }
                    out_pkt.type = DATA;
                    out_pkt.seqnum = nextSeqNum;
                    out_pkt.size = get_file_segment(nextSeqNum, file_ptr, out_pkt.data, PACKET_SIZE, file_size);
                    
                    // check if it's last packet
                    if(nextSeqNum == lastDataSeqNum)
                        out_pkt.ending_flag = 1;

                    
                    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &rcv_addr, rcv_len) == -1)
                        error("ERROR on sending file request packet after receiving ACK\n");  
                    print_packet(out_pkt, 0, 0);// send request pakcet, print data

                    nextSeqNum++;
                }  
                resend = 0;
            }                  
        } 
        // receive packet
        else if (FD_ISSET(sockfd, &readfds)) 
        {
            FD_SET(sockfd, &readfds);

            if(resend == 1){
                printf("1 ========= Timer expired, resend pkt from %d\n", base);
                //send data while there are remaining data in the window
                while (nextSeqNum < base + win_size && nextSeqNum <= lastDataSeqNum)
                {
                    if(nextSeqNum == base){ //when sending first packet

                        // setting timer
                        time_out_val.it_value.tv_sec = TIMEOUT/1000;
                        time_out_val.it_value.tv_usec = (TIMEOUT*1000) % 1000000;   
                        time_out_val.it_interval = time_out_val.it_value;
                        if (setitimer(ITIMER_REAL, &time_out_val, NULL) == -1) {
                           perror("error calling setitimer()");
                           exit(1);
                        }
                        else{
                            resend = 0;
                            printf("Reset timer because resending from %d\n", base);
                        }
                    }
                    out_pkt.type = DATA;
                    out_pkt.seqnum = nextSeqNum;
                    out_pkt.size = get_file_segment(nextSeqNum, file_ptr, out_pkt.data, PACKET_SIZE, file_size);
                    
                    // check if it's last packet
                    if(nextSeqNum == lastDataSeqNum)
                        out_pkt.ending_flag = 1;

                    
                    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &rcv_addr, rcv_len) == -1)
                        error("ERROR on sending file request packet after receiving ACK\n");  
                    print_packet(out_pkt, 0, 0);// send request pakcet, print data

                    nextSeqNum++;
                }  
                resend = 0;
            }      

            //receive data
            if (recvfrom(sockfd, &in_pkt, sizeof(in_pkt), 0, (struct sockaddr*) &rcv_addr, &rcv_len) == -1){
                error("ERROR on receiving file request packet");
            }
            print_packet(in_pkt, 1, 0); // receive request, print request

            //if received DATA
            if(in_pkt.type == DATA)
            {
                if(debug) printf("---- In select DATA\n");

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
                    if(in_pkt.ending_flag == 1)
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
                
                if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &rcv_addr, rcv_len) == -1)
                    error("ERROR on sending file requack ACK after received request\n");     
                print_packet(out_pkt, 0, 0);// send request pakcet, print data

            }
            else //if received ACK
            {
                if(debug) printf("---- In select ACK\n");

                // lose this ACK (didn't receive)
                if(simulate(loss_rate)){
                    printf("Simulate didn't get ACK %d\n", in_pkt.seqnum);
                    continue;
                }
                else if (simulate(corrupt_rate)) {
                    printf("Simulate ACK %d is corrupted\n", in_pkt.seqnum);
                    continue;
                }
                //receive acknowledgement
                if (in_pkt.seqnum >= base)
                {
                    //sliding sending window
                    base = in_pkt.seqnum + 1; //in_pkt.seqnum -> ack num

                    if(base > lastDataSeqNum){
                        printf("File transfer completed\n");
                        exit(1);
                    }    
                    // setting timer
                    time_out_val.it_value.tv_sec = TIMEOUT/1000;
                    time_out_val.it_value.tv_usec = (TIMEOUT*1000) % 1000000;   
                    time_out_val.it_interval = time_out_val.it_value;
                    if (setitimer(ITIMER_REAL, &time_out_val, NULL) == -1) {
                       perror("error calling setitimer()");
                       exit(1);
                    }
                    else{
                        resend = 0;
                        printf("Reset timer because get ACK %d\n", in_pkt.seqnum);
                    }               
                }

                //send data while there are remaining data in the window
                while (nextSeqNum < base + win_size && nextSeqNum <= lastDataSeqNum)
                {
                    
                    if(nextSeqNum == base){ //when sending first packet
                        // setting timer
                        time_out_val.it_value.tv_sec = TIMEOUT/1000;
                        time_out_val.it_value.tv_usec = (TIMEOUT*1000) % 1000000;   
                        time_out_val.it_interval = time_out_val.it_value;
                        if (setitimer(ITIMER_REAL, &time_out_val, NULL) == -1) {
                           perror("error calling setitimer()");
                           exit(1);
                        }
                        else{
                            resend = 0;
                            printf("Reset timer because resending from %d\n", base);
                        }
                    }
                    out_pkt.type = DATA;
                    out_pkt.seqnum = nextSeqNum;
                    out_pkt.size = get_file_segment(nextSeqNum, file_ptr, out_pkt.data, PACKET_SIZE, file_size);
                    
                    // check if it's last packet
                    if(nextSeqNum == lastDataSeqNum)
                        out_pkt.ending_flag = 1;

                    
                    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &rcv_addr, rcv_len) == -1)
                        error("ERROR on sending file request packet after receiving ACK\n");  
                    print_packet(out_pkt, 0, 0);// send request pakcet, print data

                    nextSeqNum++;
                }

            }
        }
        else
        {
            FD_SET(sockfd, &readfds);
            if(debug) printf("----- not in select\n");

            if(resend == 1){
                printf("2 ========= Timer expired, resend pkt from %d\n", base);
                //send data while there are remaining data in the window
                while (nextSeqNum < base + win_size && nextSeqNum <= lastDataSeqNum)
                {
                    if(nextSeqNum == base){ //when sending first packet
                        // setting timer
                        time_out_val.it_value.tv_sec = TIMEOUT/1000;
                        time_out_val.it_value.tv_usec = (TIMEOUT*1000) % 1000000;   
                        time_out_val.it_interval = time_out_val.it_value;
                        if (setitimer(ITIMER_REAL, &time_out_val, NULL) == -1) {
                           perror("error calling setitimer()");
                           exit(1);
                        }
                        else{
                            resend = 0;
                            printf("Reset timer because resending from %d\n", base);
                        }
                    }
                    out_pkt.type = DATA;
                    out_pkt.seqnum = nextSeqNum;
                    out_pkt.size = get_file_segment(nextSeqNum, file_ptr, out_pkt.data, PACKET_SIZE, file_size);
                    
                    // check if it's last packet
                    if(nextSeqNum == lastDataSeqNum)
                        out_pkt.ending_flag = 1;

                    
                    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &rcv_addr, rcv_len) == -1)
                        error("ERROR on sending file request packet after receiving ACK\n");  
                    print_packet(out_pkt, 0, 0);// send request pakcet, print data

                    nextSeqNum++;
                }  
                resend = 0;
            }      

            if(state == WAITING)
            {
                if(debug) printf("waiting %d\n", state);           
            }
            
            if(state == PROCESSING)
            {
                //if file exist, open file
                //if file doens't exist, save error message into buffer

                // check if file exist
                if(debug) printf("Process request: %s\n", rcvBuffer);

                size_t readResult;
                
                //Open file and read
                FILE* file = fopen ( rcvBuffer , "r" );

                if(file == NULL) { //If the file does not exist
                  error("404: File not found\n");
                }

                //Obtain file size:
                //fseek: Reposition stream position indicator
                //SEEK_END: End of file
                fseek (file , 0 , SEEK_END);
                file_size = ftell (file); //Get current position in stream
                rewind (file); //Set position of stream to the beginning

                // allocate memory to contain the whole file:
                file_ptr = (char*) malloc (sizeof(char)*file_size);
                if (file_ptr == NULL) error("Memory error");

                // copy the file into the buffer:
                readResult = fread (file_ptr,1,file_size,file);
                if (readResult != file_size) error("Reading error");
                  //write(newsockfd, "Reading error\r\n", 15);


                lastDataSeqNum =  file_size/PACKET_SIZE;
                if(file_size % PACKET_SIZE != 0)
                    lastDataSeqNum += 1;
                
                printf("file_size: %d\tlastDataSeqNum: %d\n", file_size, lastDataSeqNum);

                while (nextSeqNum < base + win_size && nextSeqNum <= lastDataSeqNum)
                {
                    
                    if(nextSeqNum == base){ //when sending first packet
                        // setting timer
                        time_out_val.it_value.tv_sec = TIMEOUT/1000;
                        time_out_val.it_value.tv_usec = (TIMEOUT*1000) % 1000000;   
                        time_out_val.it_interval = time_out_val.it_value;                        
                        if (setitimer(ITIMER_REAL, &time_out_val, NULL) == -1) {
                           perror("error calling setitimer()");
                           exit(1);
                        }
                        else{
                            resend = 0;
                            printf("Set timer because sending first pkt\n");
                        }
                    }
                    
                    out_pkt.type = DATA;
                    out_pkt.seqnum = nextSeqNum;
                    out_pkt.size = get_file_segment(nextSeqNum, file_ptr, out_pkt.data, PACKET_SIZE, file_size);
                    
                    // check if it's last packet
                    if(nextSeqNum == lastDataSeqNum)
                        out_pkt.ending_flag = 1;

                    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) &rcv_addr, rcv_len) == -1)
                        error("ERROR on sending file request packet\n");  
                    print_packet(out_pkt, 0, 0);// send request pakcet, print data

                    nextSeqNum++;
                }
                state = WAITING;

            }

        }
    }
    free(rcvBuffer);

    return 0; 
}

