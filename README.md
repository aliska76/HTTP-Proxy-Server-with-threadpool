# HTTP-Proxy-Server-with-threadpool
In this project implements on C ansi HTTP Proxy Server with threadpool and limited subset of the entire HTTP specification.
The proxy server gets an HTTP request from the client, and performs some predefined checks on it. If the request is found legal, it forwards the request to the appropriate web server, and sends the response back to the client. Otherwise, it send s a response to the client without sending anything to the server. Only IPv4 connections should be supported. This project written on Linux operating system on C. I hope it will help you to understand how to work connection between server and client and how work a proxy server.

Submitted Files
---------------
	proxyServer.c - an implementation for a basic TCP proxy server written in C. 
	threadpool.c - an implementation of a threadpool written in C. 
	threadpool.h - header file with stracts of a threadpool. 
	tester.sh - program with several tests written in bash. 
	README for tester.txt - file that describe how to run the tester. 
	tester_extract.sh - program that can run the tester.sh file, written in bash. 
	test0.sh - main test with compilation line in bash. 
	test#.sh - 1 - 6 tests, written in bash. 
	filter.txt  - filter with list of sites that restrained for visiting. 
	proxy-files - the folder with pdf files wich contain http errors like 400, 403, 404 and etc. Please pay close attantion that you named the folder in one word without spaces. 

proxyServer.c and threadpool.c (the executable file should be called proxyServer).

Program Description
---------------

The server should handle the connections with the clients. As we saw in class, when using
TCP, a server creates a socket for each client it talks to. In other words, there is always one socket
where the server listens to connections and for each client connection request, the server opens
another sock et. In order to enable multithreaded program, the server should create threads that
handle the connections with the clients. Since, the server should maintain a limited number of
threads, it construct a thread pool. In other words, the server create the pool of threads in advanced
and each time it needs a thread to handle a client connection, it take one from the pool or enqueue
the request if there is no available thread in the pool.

Command line usage: proxyServer <port> <pool size> <max number of request>
- Port is the port number your proxy server will listen on.
- pool size is the number of threads in the pool.
- number--of--request is the maximum number of request our server will handle before it terminates (we request is the maximum number of request your server will handle before it terminates (we count also unsuccessful requests). count also unsuccessful requests). This parameter implies that our proxy server does not run forever.This parameter implies that our proxy server does not run forever.
- Filter is ailter is an absolute path to the filter file. This file contains Host names separated by new line that the proxy will filter. You may see an example of a filter in file filter.

In this program we need to:
	• Read http -request from socket 
	• Check input: the request first line should contain method, path and protocol, and there must be a host header. Here, you only 	have to check that these tokens exist and the protocol is one of the http. Here, we only have to check that these tokens exist 		and that the protocol is one of the http versions. Other checks on the method and the path will be checked later. In case the 		request is wrong, will send 400 "Bad Request" respond, as in file 400.pdf
	• You should support only the GET method, if you get another method, return error message 501 "Method Not Implemented", as in 		file 501.pdf
	• If you can't get the IP of the target server (from the Host header), send 404 "Not Found" response, as in file 404.pdf
	• If the requestIf the request directs to a host that appears in the filterhost that -list, return error message 403 “Forbidden” 	 as in file 403.pdf
	• Otherwise, the http request is legal. The The “Host:” header contains server hostname and optionally a port, e.g. header 		contains server hostname and optionally a port, e.g. www.ynet.co.il:80. If the port does not exist, use 80 as a default port. We 	 will send the request to the server, get his response, and send it back to the client.

Few comments:
---------------
1. Our proxy server closes connection after sending the response.Our proxy server closes connection after sending the response.
2. Don't use files to senDon't use files to send error responses, this is very inefficient.
3. Each user should be handled in new thread. 
4. When you fill your sockaddr_in struct, you can use htonl(INADDR_ANY) when assigning sin_addr.s_addr, meaning that the proxy server listen to requests in any of its addresses. 
5. Don’t have to read the whole server response before sending it to the client.
6. Besides the host header, all other headers are irrelevant.

The threadpool
---------------
The pool is implemented by a queue. When the server gets a connection (getting back from accept()), it should put the connection in the queue. When there will be available thread (can be immediately), it will handle this connection (read request and write response).
We will implement the functions in threadpool.h
The server should first init the thread pool by calling the function create_threadpool(int). This function gets the size of the pool.
create_threadpool should: 
1. Check the legacy of the parameter.
2. Create threadpool structure and initialize it:
a. num_thread = given parameter
b. qsize=0
c. threads = pointer to <num_thread> threads
d. qhead = qtail = NULL
e. Init lock and condition variables.
f. shutdown = dont_accept = 0
g. Create the threads with do_work as execution function and the pool as an argument.

do_work 
-------
do_work should run in an endless loop and:
1. If destruction process has begun, exit thread
2. If the queue is empty, wait (no job to make)
3. Check again destruction flag.
4. Take the first element from the queue (*work_t)
5. If the queue becomes empty and destruction process wait to begin, signal destruction process.
6. Call the thread routine.

dispatch
---------
Dispatch gets the pool, pointer to the thread execution routine and argument to dispatch execution routine. Dispatch should:
1. Create work_t structure and init it with the routine and argument.
2. If destroy function has begun, don't accept new item to the queue
3. Add item to the queue
	*destroy_threadpool*
1. Set don’t_accept flag to 1
2. Wait for queue to become empty
3. Set shutdown flag to 1
4. Signal threads that wait on empty queue, so they can wake up, see shutdown flag and exit.
5. Join all threads
6. Free whatever you have to free.
Program flow:
1. Server creates pool of threads, threads wait for jobs.
2. Server accept a new connection from a client (aka a new socket fd)
3. Server dispatch a job - call dispatch with the main negotiation function and fd as a parameter. dispatch will add work_t item to the queue.
4. When there will be an available thread, it will takes a job from the queue and run the negotiation function.

Error handling:
--------------
1. In any case of wrong command usage, print"Usage: proxyServer <port>  <pool-size> <maxnumber-of-request> <filter>\\n".  
2. In any case of a failure before connection with client is set, use perror(“error: <sys_call>\\n”) and exit the program. 
3. In any case of a failure after the connection with client is set, and in case the error is due to some server-side error (like failure in malloc), send an 500 "Internal Server Error", as in file 500.pdf.
Don't forget to enter new line after each error message (in the first two cases).

Assumptions:
--------------
We will relate only to the method GET (always in the first line), and the header Host (the headers of HTTP request are separated from the possible message by empty line; the Host doesn't have to be the first header!). We can ignore anything else.
Purely for interest, we add the condition that we can’t visit the www.google.com:80 more than twice in one test

Compile the proxy server:
---------------
makefile:
  all:
	gcc -Wall *.c threadpool.h -o proxyServer -lpthread

Remember that you have to compile with the –lpthread flag.

Test the code:
---------------
You can use a browser. Set the proxy settings to your proxy, e.g. localhost and 8000 if you are testing the proxy on the same PC and it is listening on port 8000. This project was wrote on Ubuntu (Linux) and run in the terminal.

Tester HTTP Proxy Server:
The tester extracts, tests & logs.
In grades file you can see if you pass the tests of the tester

USAGE:
1. copy your TAR into the tester's folder (must be of the form ID_111111111_work.tar, e.g. ID_111111111_work.tar)
to create tar archive use line:
tar -cvf ID_111111111_work.tar proxyServer.c threadpool.c proxy-files filter.txt
2. run: ./tester_extract.sh ID_111111111_work.tar

Results:
1. the results log file will be generated under the logs folder
2. the tests grades will appear in grades.csv (1 = pass, 0 = fail)
