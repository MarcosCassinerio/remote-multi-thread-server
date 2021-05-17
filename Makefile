all: server client

server: 
	gcc -g -pthread -o server RemoteMultiThreadServer.c

client:
	gcc -g -pthread -o client RemoteClient.c
