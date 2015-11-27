#define PACKET_SIZE 		1024
#define TIMEOUT				100		//in miliseconds 		
#define SEND_BUFFER_SIZE	1024
#define RECEIVE_BUFFER_SIZE	PACKET_SIZE * 1024

//#define MAXWINDOWSIZE 	10

int debug = 1;

//sending side variables
int base;
int nextSeqNum;
int lastDataSeqNum;

//receiving side variable
int expSeqNum;


typedef enum {ACK, DATA} packet_type_t;

typedef struct {
  packet_type_t type;		// 1 byte ACK/DATA
  int seqnum;				// sequence number: 4 byte
  int size;					// total file size: 4 byte
  char ending_flag;
  char data[PACKET_SIZE];
}packet_t;


void print_packet(packet_t pkt, int receive_pkt, int print_data){
	if(receive_pkt == 1)		printf("Recv pkt: ");
	else if(receive_pkt == 0)	printf("Send pkt: ");
	else						printf("Lose pkt: ");

	if(pkt.type == ACK)	printf("Type: Ack \t");
	else				printf("Type: Data\t");

	printf(" %d \t size: %d \t", pkt.seqnum, pkt.size);

	if(print_data == 1)	printf("Data: %s\n", pkt.data);
	else				printf("\n");
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


int rdt_send(char* buffer, int length, int sockfd, struct sockaddr_in addr, int cwnd)
{

	// signal(SIGALRM, alarm_handler);
	/*

	initialize variables
	totalpacket = 

	int base = 1 //starting sequence number from 1

	send initial window of packets

	receive initial acknowledgement


	while (base < totalpacket)
	{
		if(select (int maxfdp, fd_set *readset, fd_set *writeset, fd_set*excepset, TIMEOUT))
		//returns the number of socket descriptor 
		{
			
			//receive acknowledgement
			if (ack_num >= base)
				sliding sending window -> base = ack_num + 1
				If(base==nextseqnum)
		            setitimer(0)
				else 
					setitimer(TIMEOUT)
			//add print statement

			//send data
			while there are remaining data in the window
				if(nextseqnum == base) //when sending first packet
					setitimer(TIMEOUT)
				build_data(DATA, buffer) //set end_flag header as necessary
				udt_send(data)
				nextseqnum++
			//add print statement
	}		
	return 0;
	*/

	return -1;

}


//return pointer to 
// char* rdt_receive(int* length, int sockfd, struct sockaddr_in addr)
// {
/*

	receive the first packet that declares the number of packets (blocking)
	send ACK

	initialize variables
	int expectedseqnum = 1;

	while(expectedseqnum < totalpacket){
		select (int maxfdp, fd_set *readset, fd_set *writeset, fd_set*excepset, TIMEOUT)
		{
			
			process_data (int& seqnum)
			if (seqnum == 1)
				initialization work
				char* buffer and dynamically allocate buffer

			if (seqnum == expectedseqnum)
				append_data(data)
				build_data(ACK, expectedseqnum) 
				udt_send(ACK, expectedseqnum)
				expectedseqnum++
			else 
				build_data(ACK, buffer) 
				udt_send(ACK, expectedseqnum-1)
		}
	}
	*/
// }

