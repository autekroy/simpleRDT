#define PACKET_SIZE 		1024
#define TIMEOUT				500	//in miliseconds 		

//state constant
#define WAITING     		0
#define PROCESSING  		1
#define RETRANSMIT  		2
#define ERROR  				3
#define TRANSMITTING		4
#define REQUEST             5

/*
Enchancements
1. Sender and receiver constantly running (do not exit)
2. Initial receiver's file request should be accomplished via rdt_send
3. If error was retransmitted, it was sent as packet type DATA instead of ERR
*/

void resend_window(int sig);

int debug = 1;

typedef enum {ACK, DATA, ERR} packet_type_t;

typedef struct {
  packet_type_t type;		// 1 byte ACK/DATA
  int seqnum;				// sequence number: 4 byte
  int size;					// total file size: 4 byte
  char ending_flag;
  char data[PACKET_SIZE];
}packet_t;

//Global variable
int state;
packet_t in_pkt, out_pkt;
double loss_rate, corrupt_rate;
struct itimerval time_out_val, time_out_cancel;

//sending side variables
int win_size;
int base;
int nextSeqNum;
int lastDataSeqNum;

//receiving side variable
int expSeqNum;

void rdt_init()
{
	expSeqNum = 1;
    base = 1;
    nextSeqNum = base;
}

void print_packet(packet_t pkt, int receive_pkt, int print_data){
	if(receive_pkt == 1)		printf("Recv pkt: ");
	else if(receive_pkt == 0)	printf("Send pkt: ");
	else						printf("Lose pkt: ");

	if(pkt.type == ACK)			printf("Type: Ack \t");
    else if(pkt.type == ERR)    printf("Type: ERR \t");
	else						printf("Type: Data\t");

	printf(" %d \t size: %d \t", pkt.seqnum, pkt.size);

	if(print_data == 1)			printf("Data: %s\n", pkt.data);
	else						printf("\n");
}

void error(char *msg)
{
    fprintf(stderr, "%s", msg);
    exit(0);
}

unsigned int get_file_segment(int nextSeqNum, char* file_ptr, char* data, int pktSize, int file_size){
	int now_pos = (nextSeqNum-1) * pktSize;

	if(nextSeqNum * pktSize <= file_size){
		memcpy(data, file_ptr + now_pos, pktSize);
		return pktSize;
	}
	else{
		int remaining = file_size - now_pos;
		memcpy(data, file_ptr + now_pos, remaining);
		return remaining;
	}

}

int simulate(double probability) {
    double random_value = (double) rand() / (double) RAND_MAX;
    if (random_value <  probability)
        return 1;
    return 0;
}

//returns -1 if data is corrupted or lost
//returns 0 if data is received
int rdt_receive_data(char* rcvBuffer, unsigned int* rcvBufferIndex, int sockfd, struct sockaddr_in* addr_ptr)
{ 
	int r = -1;

    if (simulate(loss_rate))
    {
    	if(debug) printf("Simulate DATA packet loss (seqnum %d)\n", in_pkt.seqnum);
    	return r;
    }

    if(simulate(corrupt_rate))
    {
    	if(debug) printf("Simulate DATA packet corruption (seqnum %d)\n", in_pkt.seqnum);
    }
    else if (in_pkt.seqnum == expSeqNum)
    {
        if(debug) printf("Packet seqnum %d stored\t", in_pkt.seqnum);

        memcpy(rcvBuffer + *rcvBufferIndex, &in_pkt.data, in_pkt.size);
        (*rcvBufferIndex) += in_pkt.size; 
        expSeqNum++;

        //transition state if completed receiving
        if(in_pkt.ending_flag == 1)
        {
            if (debug) printf("(last pkt)");
        }
        if (debug) printf("\n");
        r = 0;
    }
    else
    {
        if (debug) printf("***** Packet seqnum %d dropped\n", in_pkt.seqnum);
    }

    //Enhancement: create a ACK specific package to reduce ACK size
    out_pkt.type = ACK;
    out_pkt.seqnum = expSeqNum - 1;
    out_pkt.size = 0;
    addr_ptr->sin_family = AF_INET;
    if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) addr_ptr, sizeof(struct sockaddr_in)) == -1)
        error("ERROR on sending ACK packet\n");     
    print_packet(out_pkt, 0, 0);// send request packet, print data
    return r;
}

// send everything between base and nextSeqNum
void rdt_send(int sockfd, struct sockaddr_in* addr_ptr, char* file_ptr, int file_size, packet_type_t packet_type)
{
	while (nextSeqNum < base + win_size && nextSeqNum <= lastDataSeqNum)
    {
        if(nextSeqNum == base){ //when sending first packet
            // start timer
            if(debug) printf("timer started\n");
            if (setitimer(ITIMER_REAL, &time_out_val, NULL) == -1) {
               error("ERROR calling setitimer");
            }
        }
        out_pkt.type = packet_type;
        out_pkt.seqnum = nextSeqNum;
        out_pkt.size = get_file_segment(nextSeqNum, file_ptr, out_pkt.data, PACKET_SIZE, file_size);
        
        // check if it's last packet
        out_pkt.ending_flag = (nextSeqNum == lastDataSeqNum)?1:0;
        addr_ptr->sin_family = AF_INET;
        if (sendto(sockfd, &out_pkt, sizeof(out_pkt), 0, (struct sockaddr*) addr_ptr, sizeof(struct sockaddr_in)) == -1)
            error("ERROR on sending DATA packet\n");  
        print_packet(out_pkt, 0, 0);// send request pakcet, print data

        nextSeqNum++;
    }  
}


void rdt_retransmit(int sockfd, struct sockaddr_in* addr_ptr, char* file_ptr, int file_size, packet_type_t packet_type)
{
	nextSeqNum = base;
    rdt_send(sockfd, addr_ptr, file_ptr, file_size, packet_type);

    if (signal(SIGALRM, (void (*)(int)) resend_window) == SIG_ERR) {
        error("ERROR Unable to catch SIGALRM");
    }
}

//returns -1 if data is corrupted or lost
//returns 0 if data is received
int rdt_receive_ack(int sockfd, struct sockaddr_in* addr_ptr, char* file_ptr, int file_size)
{
	int r = -1;

    if (simulate(loss_rate))
    {
    	if(debug) printf("Simulate ACK packet loss (seqnum %d)\n", in_pkt.seqnum);
    }
    else if(simulate(corrupt_rate))
    {
    	if(debug) printf("Simulate ACK packet corruption (seqnum %d)\n", in_pkt.seqnum);
    }
    else if (in_pkt.seqnum >= base)
    {
        //sliding sending window
        base = in_pkt.seqnum + 1; //in_pkt.seqnum -> ack num

        // setting timer
        if(base == nextSeqNum)
        {
            if(debug) printf("timer cancelled\n");
            if (setitimer(ITIMER_REAL, &time_out_cancel, NULL) == -1) {
                error("ERROR calling setitimer()");
            }
        }
        else{
            if(debug) printf("timer started\n");
            if (setitimer(ITIMER_REAL, &time_out_val, NULL) == -1) {
                error("ERROR calling setitimer()");
            }
        } 
        r = 0;           
    }
    rdt_send(sockfd, addr_ptr, file_ptr, file_size, DATA);
    return r;
}
