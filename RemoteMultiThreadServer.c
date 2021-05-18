/* RemoteMultiThreadServer.c */
/* Cabeceras de Sockets */
#include <sys/types.h>
#include <sys/socket.h>
/* Cabecera de direcciones por red */
#include <netinet/in.h>
/**********/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
/**********/
/* Threads! */
#include <pthread.h>

/* Asumimos que el primer argumento es el puerto por el cual escuchará nuestro
servidor */

/* Maxima cantidad de cliente que soportará nuestro servidor */
#define MAX_CLIENTS 25
#define MAX_LINE 1024

typedef struct _User {
  int socket;
  char nickname[MAX_LINE];
}User;

User users[MAX_CLIENTS];

pthread_mutex_t mutex;

/* Anunciamos el prototipo del hijo */
void *child(void *arg);
/* Definimos una pequeña función auxiliar de error */
void error(char *msg);

int main(int argc, char **argv){
  int sock, *soclient, i = 0;
  struct sockaddr_in servidor, clientedir;
  socklen_t clientelen;
  pthread_t thread;
  pthread_attr_t attr;

  if (argc <= 1) error("Faltan argumentos");

  /* Creamos el socket */
  if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    error("Socket Init");

  /* Creamos a la dirección del servidor.*/
  servidor.sin_family = AF_INET; /* Internet */
  servidor.sin_addr.s_addr = INADDR_ANY; /**/
  servidor.sin_port = htons(atoi(argv[1]));

  /* Inicializamos el socket */
  if (bind(sock, (struct sockaddr *) &servidor, sizeof(servidor)))
    error("Error en el bind");

  printf("Binding successful, and listening on %s\n",argv[1]);

  /************************************************************/
  /* Creamos los atributos para los hilos.*/
  pthread_attr_init(&attr);
  /* Hilos que no van a ser *joinables* */
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
  /************************************************************/

  pthread_mutex_init(&mutex, NULL);

  for(; i < MAX_CLIENTS; ++i)
    users[i].socket = -1;

  /* Ya podemos aceptar conexiones */
  if(listen(sock, MAX_CLIENTS) == -1)
    error(" Listen error ");

  for(;;){ /* Comenzamos con el bucle infinito*/
    /* Pedimos memoria para el socket */
    soclient = malloc(sizeof(int));

    /* Now we can accept connections as they come*/
    clientelen = sizeof(clientedir);
    if ((*soclient = accept(sock
                          , (struct sockaddr *) &clientedir
                          , &clientelen)) == -1)
      error("No se puedo aceptar la conexión. ");

    /* Le enviamos el socket al hijo*/
    pthread_create(&thread , NULL , child, (void *) soclient);

    /* El servidor puede hacer alguna tarea más o simplemente volver a esperar*/
  }

  /* Código muerto */
  close(sock);

  return 0;
}

void * child(void *_arg){
  int socket = *(int*) _arg, i = 0, flag = 0, myId;
  char buf[MAX_LINE] = "", msg[MAX_LINE*2] = "", nickname[MAX_LINE] = "";

  /* SEND PING! */
  // send(socket, "PING!", sizeof("PING!"), 0);
  /* WAIT FOR PONG! */
  while(!flag) {
    send(socket, "INGRESE UN NICKNAME:", sizeof("INGRESE UN NICKNAME:"), 0);
    recv(socket, buf, sizeof(buf), 0);
    if(!strcmp(buf, "/exit"))
      break;
    if (strchr(buf, ' ') == NULL && buf[0]!='/') {
      pthread_mutex_lock(&mutex);
      for(i = 0; i < MAX_CLIENTS; ++i) {
        if(users[i].socket != -1 && !strcmp(buf, users[i].nickname)) {
          send(socket, "NICKNAME YA EN USO", sizeof("NICKNAME YA EN USO"), 0);
          break;
        }
      }
      if (i == MAX_CLIENTS) {
        flag = 1;
        for(i = 0; i < MAX_CLIENTS; ++i) {
          if(users[i].socket == -1) {
            users[i].socket = socket;
            strcpy(users[i].nickname, buf);
            break;
          }
        }
      }
      pthread_mutex_unlock(&mutex);
    } else
      send(socket, "NICKNAME INVALIDO", sizeof("NICKNAME INVALIDO"), 0);
  }

  if (!flag) {
    free((int*)_arg);
    return NULL;
  }

  myId = i;

  strcpy(msg, users[myId].nickname);
  strcat(msg, " ENTRO A LA SALA");
  msg[strlen(msg)] ='\0';

  pthread_mutex_lock(&mutex);
  for (i = 0; i < MAX_CLIENTS; ++i) {
    if (i != myId && users[i].socket != -1) 
      send(users[i].socket, msg, sizeof(msg), 0);
  }
  pthread_mutex_unlock(&mutex);
  
  send(socket, "BENVINDO", sizeof("BENVINDO"), 0);
  for (;;) {
    strcpy(nickname, "");
    strcpy(msg, "");
    recv(socket, buf, sizeof(buf), 0);
    printf("soy %d [%s] en socket %d--> Recv: %s\n", myId, users[myId].nickname, socket, buf);

    if (buf[0] != '/') {
      strcpy(msg, users[myId].nickname);
      strcat(msg, ": ");
      strcat(msg, buf);
      msg[strlen(msg)] ='\0';
      pthread_mutex_lock(&mutex);
      for (i = 0; i < MAX_CLIENTS; ++i) {
        if (i != myId && users[i].socket != -1) 
          send(users[i].socket, msg, sizeof(msg), 0);
      }
      pthread_mutex_unlock(&mutex);
    } else {
      if (!strcmp(buf, "/exit"))
        break;
      if(strlen(buf) > 10 && buf[9] == ' ' && sscanf(buf, "/nickname %s", nickname) == 1) {
        if(strcmp(users[myId].nickname, nickname)) {
          if (strchr(nickname, ' ') == NULL && nickname[0]!='/') {
            pthread_mutex_lock(&mutex);
            for(i = 0; i < MAX_CLIENTS; ++i) {
              if(users[i].socket != -1 && !strcmp(nickname, users[i].nickname)) {
                send(socket, "NICKNAME YA EN USO", sizeof("NICKNAME YA EN USO"), 0);
                break;
              }
            }
            if (i == MAX_CLIENTS) {
              strcpy(msg, users[myId].nickname);
              strcat(msg, " CAMBIO SU NICKNAME A ");
              strcat(msg, nickname);
              msg[strlen(msg)] ='\0';
              for (i = 0; i < MAX_CLIENTS; ++i) {
                if (i != myId && users[i].socket != -1) 
                  send(users[i].socket, msg, sizeof(msg), 0);
              }
              strcpy(users[myId].nickname, nickname);
              send(socket, "NICKNAME ACTUALIZADO", sizeof("NICKNAME ACTUALIZADO"), 0);
            }
            pthread_mutex_unlock(&mutex);
          } else 
            send(socket, "NICKNAME INVALIDO", sizeof("NICKNAME INVALIDO"), 0);
        } else
          send(socket, "NO SE PUEDE CAMBIAR AL NICKNAME ACTUAL", sizeof("NO SE PUEDE CAMBIAR AL NICKNAME ACTUAL"), 0);
      } else if (strlen(buf) > 7 && buf[4] == ' ' && sscanf(buf, "/msg %s %s", nickname, msg) == 2) {
        if (strcmp(msg, "") != 0) {
          
        } else 
          send(socket, "EL MENSAJE NO PUEDE ESTAR VACIO", sizeof("EL MENSAJE NO PUEDE ESTAR VACIO"), 0);
      } else
        send(socket, "COMANDO INVALIDO", sizeof("COMANDO INVALIDO"), 0);
    }
  }
  users[i].socket = -1;
  free((int*)_arg);
  return NULL;
}

void error(char *msg){
  exit((perror(msg), 1));
}
