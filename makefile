all:
	gcc -Wall -pthread *.c threadpool.h -o proxyServer
