all:
	gcc -o sender sender.c
	gcc -o receiver receiver.c
run:	
	./sender 8000
	./receiver 127.0.0.1 8000 ted.jpg
clean:
	rm -f ./sender
	rm -f ./receiver
