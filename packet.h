#define MAX_SIZE 1024

struct packet {
  char type;// 1 byte
  int seq;	// sequence number: 4 byte
  int size;	// file size: 4 byte
  char data[PACKET_SIZE];
};

