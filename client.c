#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Sintaxa : %s <adresa_server> <port>\n", argv[0]);
    return -1;
  }

  /* stabilim portul de conectare la server */
  int port = atoi(argv[2]);

  int sd;

  /* cream socketul */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Eroare la socket().\n");
    return errno;
  }

  struct sockaddr_in server;

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons(port);

  /* ne conectam la server */
  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect().\n");
    return errno;
  }

  char msg[256] = "";

  if (read(sd, msg, 256) <= 0)
  {
    perror("Nu s-a putut citi cererea de citire a numelui de la server");
    return errno;
  }

  printf("%s", msg);
  fflush(stdout);
  bzero(msg, 256);

  char response[100] = "";

  read(0, response, 100);

  if (write(sd, response, 100) <= 0)
  {
    perror("Nu s-a putut face scrierea numelui catre server.");
    return errno;
  }

  printf("Ati fost inregistrat cu numele : %s\n", response);
  fflush(stdout);

  bzero(response, 100);

  printf("Asteptati inceperea jocului...\n");
  fflush(stdout);

  while (1)
  {
    if (read(sd, msg, 256) <= 0)
    {
      perror("Nu s-a putut face citirea intrebarii si variantelor de la server.");
      return errno;
    }

    printf("\n%s", msg);
    fflush(stdout);

    if (msg[0] == 2)
      break;

    if (msg[0] == 1)
    {
      printf("Ai fost eliminat din concurs");
      fflush(stdout);
      read(0, response, 100);
      return 0;
    }

    bzero(msg, 256);

    while (1)
    {
      printf("\nIntroduceti raspunsul : ");
      fflush(stdout);
      // fflush(stdin);
      read(0, response, 100);

      if (response[0] != 'a' && response[0] != 'b' && response[0] != 'c' && response[0] != 'd')
      {
        printf("Nu exista aceasta varianta de raspuns.");
      }
      else
        break;

      bzero(response, 100);
    }

    if (write(sd, response, 1) <= 0)
    {
      perror("[client]Eroare la write() spre server.\n");
      return errno;
    }
  }

  bzero(msg, 256);

  if (read(sd, msg, 256) <= 0)
  {
    perror("Nu s-a putut citi punctajul de la server");
    return errno;
  }
  printf("%s", msg);
  fflush(stdout);

  bzero(msg, 256);

  if (read(sd, msg, 256) <= 0)
  {
    perror("Nu s-a putut citi punctajul de la server");
    return errno;
  }
  printf("%s", msg);
  fflush(stdout);

  bzero(msg, 256);

  read(0, response, 100);

  close(sd);
}