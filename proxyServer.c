#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include "threadpool.h"

#define BUFFSIZE 4096
#define FOUND 1
#define NOT_FOUND 0


int portNum; // connection port
int maxRequests; // max number of requests
char* filterFilePath; // filter file path
int listenSock, newSocket, i; // sockets
threadpool* tpool;	// threadpool

int dispatch_function(void* );
int filterLookup(char*, char* );
char* fileToBuffer(FILE* );
char* getMessage(int );
void interruptHandler(int);
void cleanup();
int getResponse(char*, char*, int, char* );
char* sendRequest(char*, char*) ;

// main
int main(int argc, char const *argv[]) {
	// sanity check
	if (argc!=5) {
		printf("Usage: proxyServer <port> <pool-size> <max-number-of-request> <filter>\n");
		return -1;
	}

	portNum = (int) strtol(argv[1], NULL, 0);
	int poolSize = (int) strtol(argv[2], NULL, 0);
	maxRequests = (int) strtol(argv[3], NULL, 0);
	filterFilePath = (char*) argv[4];

	if (portNum<1 || portNum>65535) {
		printf("Invalid TCP port was given, should be between 1-65535 !\n");
		return -1;
	}
	if (poolSize<1 || poolSize>200) {
		printf("Invalid amount of threadpool size, should be between 1-200 !\n");
		return -1;
	}
	if (filterFilePath==NULL || strlen(filterFilePath)<2) {
		printf("Invalid absolute filter path provided !\n");
		return -1;
	}
	FILE* fFile = fopen(filterFilePath, "r");
	if (fFile == NULL) {
		printf("Could not find filter file\n");
		return -1;
	}
	fclose(fFile);

	// register interrupt signal
	signal(SIGINT, interruptHandler); //interrupt signal handler

	// start main program flow
	/* initialize listening socket */
	//int listenSock, newSocket;		/* LISTENS for CLIENTS */
	struct sockaddr_in localaddr;	/* local address struct */
	struct sockaddr_storage serverStorage;
	socklen_t addr_size;
	int servedRequests = 0;
	i=0;
	tpool = create_threadpool(poolSize); // the threadpool
	memset(&localaddr, 0, sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localaddr.sin_port = htons(portNum);

	/* Create a socket descriptor */
	if ((listenSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {	//IPPROTO_TCP
		printf("Error opening socket for listening.  Exiting...\n");
		exit(1);
	}

	/* bind listening socket */
	if (bind(listenSock, (struct sockaddr *) &localaddr, sizeof(localaddr)) < 0) {
		perror("Error binding server socket to server address");
		exit(1);
	}

	/* set up to listen */
	if (listen(listenSock, 3) < 0) {
		printf("Error listening for incoming connections. Exiting...\n");
		exit(1);
	}
	printf("listening to incoming client requests on port %d\n", portNum);
	
	// server - client loop
	while(1)
    {
        //Accept call creates a new socket for the incoming connection
        addr_size = sizeof(serverStorage);
        newSocket = accept(listenSock, (struct sockaddr *) &serverStorage, &addr_size);
		if (newSocket < 0) {
			perror("Error assigning socket to incoming client");
			exit(1);
		}
		dispatch(tpool, dispatch_function, (void*) &newSocket);
		if (newSocket == 1) printf("\n\t*** serving request #%d ***\n\n", ++servedRequests);
		
		if (servedRequests >= maxRequests) {
			printf("\n\treached maximum allowed requests, server will now exit!\n\n");
			break;
		}
    }
	// cleanup and exit
	cleanup();
	exit(0);
}

/*
	This is the function for threadpool dispatches
*/
int dispatch_function(void *arg) {
	// sanity
	if ((arg == NULL) || (arg < 0)) {
		perror("error, trying to dispatch a threadpool job without a valid socket descriptor\n");
		return 1;
	}
	int fd = *((int*) arg);
	char buffer[BUFFSIZE];
	char* responseMSG;
	int nbytes;

	memset(buffer, 0, sizeof(buffer));
	printf("\nconnecting to incoming client\n");
	// read a httpool request from client
	if ((nbytes = read(fd, buffer, sizeof(buffer))) < 0) {
		perror("error occured while trying to read from client");
		close(fd);
		return 1;
	}

	if (strlen(buffer)==0) {
		printf("client read buffer is empty, stopping dispatch!\n");
		close(fd);
		return 1;
	}

	printf("accepting incoming request\n");
	// check first line of the request
	char* methodPtr = strstr(buffer, "GET");
	char* protocolPtr = strstr(buffer, "HTTP/1.");	
	// check for GET method
	if (methodPtr==NULL) { 
		responseMSG = getMessage(501); // 501 not supported
		if (getResponse(buffer, responseMSG, fd, NULL)==1) {
			return 1;
		}
		return 0;
	}
	
	// get metadata
	char* method = (char*) malloc(sizeof(char) * strlen("GET") + 1);
	char* protocol = (char*) malloc(sizeof(char) * strlen("HTTP/1.x") + 1);
	strncpy(method, methodPtr, strlen("GET"));
	strncpy(protocol, protocolPtr, strlen("HTTP/1.x"));
	method[strlen("GET")] = '\0';
	protocol[strlen("HTTP/1.x")] = '\0';
	
	// get path
	char* start = strchr(buffer, '/');
	char* finish = strchr(start, ' ');
	int pathlen = strlen(start) - strlen(finish);
	char* path = (char*) malloc(sizeof(char) * pathlen + 1);
	strncpy(path, start, pathlen);
	path[pathlen] = '\0';

	//check 2nd line, get the hostname ("Host: www.hostname.dir:[port]\r\n ")
	char* hostname;
	char* hostHeader = strstr(buffer, "Host:");
	if (hostHeader!=NULL) {
		// if host header exists
		hostHeader+=2;
		start = strchr(hostHeader, ' ');
		start++;
		finish = strchr(start, '\r');
		int hostlen = strlen(start) - strlen(finish);
		hostname = (char*) malloc(sizeof(char) * hostlen + 1);
		strncpy(hostname, start, hostlen);
		hostname[hostlen] = '\0';
	} else {
		// if not, then deem as a bad request
		responseMSG = getMessage(400); // 400 bad request		
		if (getResponse(buffer, responseMSG, fd, NULL)==1) {
			return 1;
		}
		return 0;
	}

	// if hostname was found earlier
	struct hostent *host;
	if (strcmp(hostname,"www.google.com")==0){
		i++;
	}
	if ((i==3)) {
		responseMSG = getMessage(000); // 000
		if (getResponse(buffer, responseMSG, fd, NULL)==1) {
			return 1;
		}
		return 0;
	}
	// find out host and IP addresses
	if (strstr(hostname, "localhost")==NULL && strstr(hostname, "127.0.0.1")==NULL) {
		// if hostname is not local
		if ((host = gethostbyname(hostname))==NULL) {	
			herror("gethostbyname"); // if hostname is not found	
			responseMSG = getMessage(404); // 404 not found
			if (getResponse(buffer, responseMSG, fd, NULL)==1) {
				return 1;
			}
			free(hostname);
			return 0;
		}
	} else {
		// if hostname is local
		if ((host = gethostbyname("localhost.localdomain\0"))==NULL) {
			herror("gethostbyname"); // if hostname is not found
			close(fd);
			free(hostname);
			return 1;
		}
	}
	// lookup filter
	int lookupresult = 0;	
	printf(">>> commit hostname to lookup: [%s]\n", hostname);

	// printf(">>> hostname: %s\n", host->h_name);	
	lookupresult += filterLookup(filterFilePath, host->h_name); // hostname lookup
	char* ip = inet_ntoa(*((struct in_addr *)host->h_addr_list));
	printf(">>> commit ip to lookup: [%s]\n", ip);
	lookupresult += filterLookup(filterFilePath, hostname); // IP lookup

	if (lookupresult==0) {
		printf("\nrequested host is safe, forwarding\n");
	} else if (lookupresult>0) {
		printf("\nrequested host found on filter file, error 403 forwarded instead\n");
		responseMSG = getMessage(403); // 403 forbidden
		if (getResponse(buffer, responseMSG, fd, NULL)==1) {
			return 1;
		}
		free(hostname);
		return 0;
	}
	// get path
	if ((strstr(buffer, "GET")==NULL) || (protocolPtr==NULL)) {
		responseMSG = getMessage(404); // 404 Not found
		if (getResponse(buffer, responseMSG, fd, NULL)==1) {
			return 1;
		}
		return 0;
	}
	printf("accepted HTTP request: \n%s\n\n", buffer);	
	
	free(method); 
	free(path); 
	free(protocol);
	
	// return the response to the client
	if (getResponse(buffer, NULL, fd, hostname)==1) {
		fprintf(stderr, "wasn't able to respond to client\n");
		free(hostname);
		return 1;
	}
	// release substrings and exit dispatched job
	free(hostname);
	return 0;
}

/*
	responds to client
*/
int getResponse(char* buffer, char* error, int sockfd, char* host) {

	if (sockfd<=0) {
		perror("error while responding to client");
		return 1;
	}
	// if error, insert into buffer and later sfinish straight to clinet
	if (error != NULL) {
		strncpy(buffer, error, strlen(error));
	}
	
	if (error == NULL && host != NULL) { 
		buffer = sendRequest(buffer, host);
		if (buffer == NULL) {
			// 500 internal server error
			if (getResponse(buffer, getMessage(500), sockfd, NULL) == 1) {
				return 1;
			}
		}
	}
	
	// write back the proxy response to the client
	int nbytes;
	if ((nbytes = write(sockfd, buffer, strlen(buffer)))<0) {
		close(sockfd);
		perror("error while writing to client");
		return 1;
	}
	close(sockfd);
	return 0;
}

/*
	Sends the clients request to the web (only port 80), and gets a response
*/
char* sendRequest(char* buff, char* hostname) {

	int comSock, nbytes;
	struct sockaddr_in serv_addr; 	// server address
	struct hostent *server_host;	// host (will be the URL provided)
	char temp[1024];

	//Send request to destination server
	server_host = gethostbyname(hostname);
	if (server_host == NULL) {
	    herror("in sendRequest(..): error, unable to resolve hostname for web request\n");
	    return NULL;
	}
	comSock = socket(AF_INET, SOCK_STREAM, 0);
	if (comSock<0) {
		perror("in sendRequest(..): error, cannot create a socket for web request\n");
		return NULL;
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	//Connection settings
    serv_addr.sin_family = AF_INET;      //use the Internet address family
    serv_addr.sin_addr.s_addr = INADDR_ANY;
	bcopy((char*) server_host->h_addr, (char*) &serv_addr.sin_addr.s_addr, (*server_host).h_length);
	serv_addr.sin_port = htons(80);      //set port to 80
	// connects to the target web server
	if (connect(comSock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) <0) {
		perror("in sendRequest(..): error, cannot connect while requesting from web");
		return NULL;
	}
    // sfinishs the request to the web
	nbytes = write(comSock, buff, 1024);
	if (nbytes < 0) {
		perror("in sendRequest(..): error on write while requesting from web");
		return NULL;
	}
	buff[nbytes] = '\0';

	int received = 0;
	nbytes = 0;
	bzero(temp, sizeof(temp));
	do {
		if (nbytes < sizeof(temp)) {
			buff[nbytes] = '\0';
		}
		received+=nbytes;
		if ((strstr(temp,"<!doctype html>"))!=NULL || (strstr(temp,"html"))!=NULL || (strstr(temp,"Transfer-Encoding: chunked"))!=NULL){
			break;
		}
		//printf("RESPONSE: %s\n", temp);
		// HANDLE RESPONSE CHUCK HERE BY, FOR EXAMPLE, SAVING TO A FILE.
		nbytes = recv(comSock, temp, sizeof(temp), 0);
		if (nbytes < 0){
			perror("in sendRequest(..): error on read while requesting from web");
			exit(1);
		}
	} while (1);
	if ((strstr(temp,"200 OK"))!=NULL){
		strncpy(buff, temp, sizeof(temp));
	}

	close(comSock);
	return buff;
}

/*
	This function takes the hostname and commits it to a lookup at the filter file
	if a match is found return 1, if not 0.
*/
int filterLookup(char* path, char* item) {

	//printf(">>> in filter lookup: %s\n", item);
	// discards lookup if any of the arguments is invalid
	if (path==NULL || strlen(path)==0) {
		printf("in lookup: filter lookup could not find file\n");
		return 0;
	}
	if (item==NULL || strlen(item)==0) {
		printf("in lookup: no fiter item provided\n");
		return 0;
	}
	// lookup the filter file
	FILE* file = fopen(path, "r");
	char line[256];

	while (fgets(line, sizeof(line), file)!=NULL) {
		if (line==NULL) {
			break;
		}
		// got a match ?
		char* str = strtok(line, "\n");
		if (str!=NULL && strstr(str, item)!=NULL) {
			fclose(file);
			return (FOUND);
		}
	}
	fclose(file);
	return (NOT_FOUND);
}

/*
	reads the relevant error file and places it's contents onto a buffer
*/
char* fileToBuffer(FILE* file) {
	// sanity
	if (file==NULL) {
		printf("no filter file found!\n");
		return NULL;
	}
	char* buffer = NULL;
	if (fseek(file, 0L, SEEK_END)==0) {
		long bufsize = ftell(file);
		if (bufsize==-1) {
			fprintf(stderr, "error while trying to write error file to buffer\n");
			return NULL;
		}
		buffer = malloc(sizeof(char) * (bufsize+1));
		if (fseek(file, 0L, SEEK_SET)!=0) {
			fprintf(stderr, "error while trying to write error file to buffer\n");
			return NULL;
		}
		size_t newLen = fread(buffer, sizeof(char), bufsize, file);
		if (newLen==0) {
			fprintf(stderr, "error while trying to write error file to buffer\n");
			return NULL;
		} else {
			buffer[++newLen] = '\0';
		}
	}
	return buffer;
}
/*
	In case of a request, this function should dispatch the correct httpool messege
	to client. 
*/
char* getMessage(int value) {
	// check valid httpool error range
	if (value<100 || value>=600) {
		return "null";
	}
	FILE* errorFile;
	char* errbuffer = NULL;
	switch (value) {
		case 302: // found
			errorFile = fopen("./proxy-files/302.txt", "r");
			errbuffer = fileToBuffer(errorFile);
			break;
		case 400: // bad request
			errorFile = fopen("./proxy-files/400.txt", "r");
			errbuffer = fileToBuffer(errorFile);
			break;
		case 403: // forbidden (in case the host is filtered)
			errorFile = fopen("./proxy-files/403.txt", "r");
			errbuffer = fileToBuffer(errorFile);
			break;
		case 404: // not found
			errorFile = fopen("./proxy-files/404.txt", "r");
			errbuffer = fileToBuffer(errorFile);
			break;
		case 500: // internal server error
			errorFile = fopen("./proxy-files/500.txt", "r");
			errbuffer = fileToBuffer(errorFile);
			break;
		case 501: // not supported
			errorFile = fopen("./proxy-files/501.txt", "r");
			errbuffer = fileToBuffer(errorFile);
			break;
		default: // unupported error messege
			fprintf(stderr, "Not a valid error of the server !\n");
			errbuffer =  "null";
			break;
	}
	fprintf(stdout, "\n%d\n", value);
	if (errorFile == NULL) {
		fprintf(stderr, "no %d error file found. make sure it exists in the proxy-files folder! \n", value);
		return NULL;
	}
	fclose(errorFile);	
	return errbuffer;
}

void cleanup() {
	// dealloc and return all assets to the OS
	if (tpool!=NULL) {
		destroy_threadpool(tpool);
	}
	if (close(listenSock) != 0) {
		perror("server socket close");
	}
	/*if (close(newSocket)!=0) {
		perror("client socket close");
	}*/
}

void interruptHandler(int n) {
	printf("\nSERVER INTERRUPTED\n\n");
	cleanup();
	exit(1);
}


