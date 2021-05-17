/* RemoteClient.c
   Se introducen las primitivas necesarias para establecer una conexión simple
   dentro del lenguaje C utilizando sockets.
*/
/* Cabeceras de Sockets */
#include <sys/types.h>
#include <sys/socket.h>
/* Cabecera de direcciones por red */
#include <netdb.h>
/**********/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>

#define MAX_LINE 1024
/*
  El archivo describe un sencillo cliente que se conecta al servidor establecido
  en el archivo RemoteServer.c. Se utiliza de la siguiente manera:
  $cliente IP port
 */

struct addrinfo *resultado;
pthread_t thread;
int sock;

void error(char *msg){
  exit((perror(msg), 1));
}

void *sendMesseges() {
  char buf[MAX_LINE];

  for (;;) {
    scanf(" %[^\n]", buf);
    
    if (!strcmp(buf, "/exit"))
      break;

    send(sock, buf, sizeof(buf),0);
  }

  kill(getpid(), SIGINT);

  return NULL;
}

void handler(int sig) {
  pthread_cancel(thread);

  send(sock, "/exit", sizeof("/exit"), 0);

  assert(!pthread_join(thread, NULL));

  freeaddrinfo(resultado);
  close(sock);

  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  char buf[MAX_LINE];

  /*Chequeamos mínimamente que los argumentos fueron pasados*/
  if(argc != 3){
    fprintf(stderr,"El uso es \'%s IP port\'", argv[0]);
    exit(1);
  }

  /* Inicializamos el socket */
  if( (sock = socket(AF_INET , SOCK_STREAM, 0)) < 0 )
    error("No se pudo iniciar el socket");

  /* Buscamos la dirección del hostname:port */
  if (getaddrinfo(argv[1], argv[2], NULL, &resultado)){
    fprintf(stderr,"No se encontro el host: %s \n",argv[1]);
    exit(2);
  }

  if(connect(sock, (struct sockaddr *) resultado->ai_addr, resultado->ai_addrlen) != 0)
    /* if(connect(sock, (struct sockaddr *) &servidor, sizeof(servidor)) != 0) */
    error("No se pudo conectar :(. ");

  printf("La conexión fue un éxito!\n");

  signal(SIGINT, handler);

  assert(!pthread_create(&thread, NULL, sendMesseges, NULL));

  for(;;) {
    recv(sock, buf, sizeof(buf),0);

    printf("%s", buf);
  }

  return 0;
}
