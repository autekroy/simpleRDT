#define PACKET_SIZE 1024
#define TIMEOUT 10

int base;
int nextseqnum;

typedef enum {ACK, DATA} packet_type_t;

typedef struct {
  packet_type_t type;// 1 byte ACK/DATA
  int seqnum;	// sequence number: 4 byte
  int size;	// file size: 4 byte
  char data[PACKET_SIZE];
}packet_t;


void print_packet(packet_t pkt, int receive_pkt, int print_data){
	if(receive_pkt == 1)printf("Recv pkt: ");
	else				printf("Send pkt: ");

	if(pkt.type == ACK)	printf("Type: Ack \t");
	else				printf("Type: Data\t");

	printf("seqnum: %d \t size: %d \t", pkt.seqnum, pkt.size);

	if(print_data == 1)	printf("Data: %s\n", pkt.data);
	else				printf("\n");
}

//converting from packet_t to stream
char* packetToStream(packet_t* packet)
{
	char* stream; 
	return stream;
}

//converting stream to packet_t
packet_t streamToPacket(char* stream)
{
	packet_t packet; 
	return packet;
}


void alarm_handler(int sig)
{
	//retransmit everything in my window
}

char* build_packet()
{
	char* tmp = "none";
	return tmp;
}



int rdt_send(char* buffer, int length, int sockfd, struct sockaddr_in addr)
{

	signal(SIGALRM, alarm_handler);
	/*

	initialize
	send initial window of packets


	while (base < length)
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
			
			//send data
			while there are remaining data in the window
				if(nextseqnum == base) //when sending first packet
					setitimer(TIMEOUT)
				build_data(DATA, buffer) //set end_flag header as necessary
				udt_send(data)
				nextseqnum++
	}
	*/
	return -1;
}




/*
//receiver
rdt_receive()
char* buffer;
int expectedseqnum;
while(1){
select (int maxfdp, fd_set *readset, fd_set *writeset, fd_set*excepset, TIMEOUT)
process_data (int& ending_flag, int& seqnum)
if (ending_flag == 1) break;

if (seqnum == expectedseqnum)
append_data(data)
build_data(ACK, buffer) 
udt_send(ACK, expectedseqnum)
expectedseqnum++
else 
	build_data(ACK, buffer) 
udt_send(ACK, expectedseqnum-1)
}
*/
