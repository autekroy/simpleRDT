all:
	gcc -o sender sender.c
	gcc -o receiver receiver.c
clean:
	rm -f ./sender
	rm -f ./receiver
