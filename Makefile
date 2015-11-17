all:
	gcc -o server server.c
	gcc -o client client.c
run:	
	./server 8000
clean:
	rm -f ./server
	rm -f ./client
