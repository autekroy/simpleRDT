#define PACKET_SIZE 	1024
#define TIMEOUT			500		//in miliseconds 		
//#define MAXWINDOWSIZE 	10

int base;
int nextseqnum;

typedef enum packet_type{ACK, DATA}packet_type_t

typedef struct packet {
  packet_type_t type;// 1 byte ACK/DATA
  int seqnum;	// sequence number: 4 byte
  int size;	// total file size: 4 byte
  char data[PACKET_SIZE];
}packet_t;

//converting from packet_t to stream
char* packetToStream(packet_t* packet)
{
	char* stream; 
	return stream;
}

//converting stream to packet_t
packet_t* streamToPacket(char* stream)
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

}



int rdt_send(char* buffer, int length, int sockfd, struct sockaddr_in addr, int cwnd)
{

	signal(SIGALRM, alarm_handler);
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


}


//return pointer to 
char* rdt_receive(int* length, int sockfd, struct sockaddr_in addr)
{
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
}
*/
