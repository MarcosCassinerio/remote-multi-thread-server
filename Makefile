all: server client

server: RemoteMultiThreadServer.c
	gcc -g -pthread -o server RemoteMultiThreadServer.c

client: RemoteClient.c
	gcc -g -pthread -o client RemoteClient.c
