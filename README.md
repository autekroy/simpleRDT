# UCLA CS 118 - project 2: simple RDT 

### Please see our report file for more information.

#### Files
1. server code: server.c
2. client code: client.c

#### Authors
1. Yao-Jen Chang 704-405-423 autekwing@ucla.edu
2. Yuanchen Liu 503-392-352 yuanchen@ucla.edu

#### Run
1. How to compile: simply type `make`
2. How to remove executable files: type `make clean` 
3. How to run: 
  * ./sender <portnumber> <window size>
  * ./receiver <sender hostname> <sender portnumber> <filenam>
4. For example: `./sender 8000 10` and `./receiver 127.0.0.1 8000 ted.jpg`


#### Reference
1. UDP socket example: http://www.binarytides.com/programming-udp-sockets-c-linux/