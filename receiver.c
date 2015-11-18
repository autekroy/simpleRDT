
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

    
    /*
    pseudo code:
    send the request via RDTsend()
    receive the data via RDTreceive()

    NextSeqNum=InitialSeqNumber
    SendBase=InitialSeqNumber
    loop (forever) 
    { 
        switch(event)
            event: send data
                create TCP segment with sequence number NextSeqNum 
                if (timer currently not running)
                    start timer
                rdt_send()
                break;
            event: timer timeout
                retransmit not-yet-acknowledged segment with
                    smallest sequence number start timer
                    rdt_send()
                break;
            event: ACK received, with ACK field value of y 
                if (y > SendBase) {
                    SendBase=y
                    if (there are currently any not-yet-acknowledged segments)
                        start timer 
                }
                break;
    } 
    
    //sender specific 
    rdt_send()
        if(nextseqnum<base+N)
        {
            sndpkt[nextseqnum]=make_pkt(nextseqnum,data,checksum) 
            udt_send(sndpkt[nextseqnum])
            if(base==nextseqnum)
                start_timer
            nextseqnum++
        } 
        else
        ￼   refuse_data(data)
    
    rdt_receive_ack()
        base=getacknum(rcvpkt)+1
        If(base==nextseqnum)
            stop_timer
        else
            start_timer
    
    //receiver specific
    rdt_receive_data()
        extract(rcvpkt,data)
        append_data(data) 
        sndpkt=make_pkt(expectedseqnum,ACK,checksum) 
            udt_send(sndpkt)
        expectedseqnum++


    */  
    return 0; 
}

