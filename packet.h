#define PACKET_SIZE 1024

typedef struct packet {
  char type;// 1 byte
  int seq;	// sequence number: 4 byte
  int size;	// file size: 4 byte
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

