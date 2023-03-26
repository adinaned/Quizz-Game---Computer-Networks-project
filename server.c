#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <stdbool.h>
#include <fcntl.h>

/* portul folosit */
#define PORT 2024

#define SECONDS_PER_MESSAGE 5
#define NR_QUESTIONS 3

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData
{
    int idThread;
    int client;
    int score;
    char nameOfClient[100];
} thData;

thData *td;
pthread_t th[100];

sqlite3 *db;

time_t timeQuestion_t, startGame_t, startAccept_t;
int currentQuestion = -1, numberOfClients = 0;
bool gameOver, lock;
int maxScore = 0, numberOfWinners;
char winnerName[100];

/**
 *  Function: callback
 *  -----------------------------
 *  used to execute the query
 *  returns the result of the query
 * 
 *  data : pointer to a linked list of names
 *  argc : the number of columns in the result set
 *  argv : the row's data
 *  azColName : columns' names
*/
static int callback(void *data, int argc, char **argv, char **azColName)
{
    for (int i = 0; i < argc; i++)
    {
        strcat(data, argv[i]);
    }

    return 0;
}

/**
 *  Function: toString
 *  -----------------------------
 *  converts a number into a string
 * 
 *  s : the string containing x
 *  x : the number that is to be converted
*/
void toString(char *s, int x)
{
    int counter = 0;
    char aux;

    if (x == 0)
    {
        s[0] = '0';
        s[1] = 0;
        return;
    }

    while (x)
    {
        s[counter++] = x % 10 + '0';
        x = x / 10;
    }

    for (int i = strlen(s) / 2; i >= 0; i--)
    {
        aux = s[i];
        s[i] = s[strlen(s) - i - 1];
        s[strlen(s) - i - 1] = aux;
    }
}

/**
 *  Function: questions
 *  -----------------------------
 *  gets the questions from the database
 * 
 *  i : the row number
 *  temp : contains the i question from the database
*/
void questions(char *temp, int i)
{
    char query[100], numberOfQuestion[4], *messageError = 0;
    int rc;

    bzero(numberOfQuestion, 4);
    bzero(query, 100);
    bzero(temp, 256);

    strcpy(query, "SELECT intrebare FROM quiz LIMIT 1 OFFSET ");
    toString(numberOfQuestion, i);
    strcat(query, numberOfQuestion);
    strcat(query, ";");

    rc = sqlite3_exec(db, query, callback, temp, &messageError);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", messageError);
        sqlite3_free(messageError);
    }
}

/**
 *  Function: correctAnswer
 *  -----------------------------
 *  gets the correct answer from the database
 * 
 *  i : the row number
 *  temp : contains the correct answer of question i
*/
void correctAnswer(char *temp, int i)
{
    char query[100], numberOfQuestion[4], *messageError = 0;
    int rc;

    bzero(numberOfQuestion, 4);
    bzero(query, 100);
    bzero(temp, 256);

    strcpy(query, "SELECT raspunsCorect FROM quiz LIMIT 1 OFFSET ");
    toString(numberOfQuestion, i);
    strcat(query, numberOfQuestion);
    strcat(query, ";");

    rc = sqlite3_exec(db, query, callback, temp, &messageError);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", messageError);
        sqlite3_free(messageError);
    }
}

/**
 *  Function: answers
 *  -----------------------------
 *  gets the possbile answers from the database
 * 
 *  i : the row number
 *  temp : contains the possbile answers of question i
*/
void answers(char *temp, int i)
{
    char query[100], numberOfQuestion[4], *messageError = 0;
    int rc;

    bzero(numberOfQuestion, 4);
    bzero(query, 100);
    bzero(temp, 256);

    strcpy(query, "SELECT variantaA||' '||variantaB||' '||variantaC||' '||variantaD FROM quiz LIMIT 1 OFFSET ");
    toString(numberOfQuestion, i);
    strcat(query, numberOfQuestion);
    strcat(query, ";");

    rc = sqlite3_exec(db, query, callback, temp, &messageError);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", messageError);
        sqlite3_free(messageError);
    }
}

/**
 *  Function: treat
 *  -----------------------------
 *  manages threads
 * 
 *  arg : structure that contains information about each thread
 *  msg : contains the data that is sent to the client
 *  response : contains the data that is received from the client 
*/
static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);

    char msg[256] = "";
    numberOfClients += 1;

    strcpy(msg, "Introduceti-va numele : ");

    if (write(tdL.client, msg, 256) <= 0)
    {
        perror("Nu s-a putut face scrierea mesajului catre client.");
        pthread_exit(0);
    }

    bzero(msg, 256);

    char response[100] = "";

    if (read(tdL.client, response, 100) <= 0)
    {
        perror("Nu s-a putut citi numele clientului\n");
        pthread_exit(0);
    }

    response[strlen(response) - 1] = '\0';
    strcpy(tdL.nameOfClient, response);

    bzero(msg, 256);
    bzero(response, 100);

    int rc = sqlite3_open("quiz.db", &db);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
    }

    char temp[256] = "";

    for (int i = 0; i <= NR_QUESTIONS; i++)
    {

        while (currentQuestion < i)
            ;

        printf("Se trimite intrebarea %d\n", i);
        fflush(stdout);

        questions(temp, i);

        strcpy(msg, temp);
        bzero(temp, 256);

        answers(temp, i);

        strcat(msg, "\n");
        strcat(msg, temp);
        strcat(msg, "\n");
        bzero(temp, 256);

        if (write(tdL.client, msg, 256) <= 0)
        {
            perror("Nu s-a putut face scrierea intrebarii si variantelor de la server.\n");
            pthread_exit(0);
        }

        bzero(msg, 256);

        if (read(tdL.client, response, 1) <= 0)
        {
            perror("[client]Eroare la read() spre server.\n");
            pthread_exit(0);
        }

        if (currentQuestion > i)
        {
            write(tdL.client, "\1", 1);
            pthread_exit(0);
        }

        correctAnswer(temp, i);

        if (response[0] == temp[0])
        {
            tdL.score += 1;
        }

        bzero(temp, 256);
        bzero(response, 100);

        if (i + 1 > NR_QUESTIONS)
        {
            bzero(msg, 256);
            strcpy(msg, "\2");
            write(tdL.client, msg, 256);
        }
    }

    char score[2];
    bzero(score, strlen(score));
    toString(score, tdL.score);

    strcpy(msg, "Punctajul obtinut este egal cu : ");
    strcat(msg, score);

    bzero(score, 2);

    while (lock)
        ;
    lock = 1;

    if (tdL.score > maxScore)
    {
        numberOfWinners = 1;
        maxScore = tdL.score;
        bzero(winnerName, 100);
        strcpy(winnerName, tdL.nameOfClient);
    }
    else if (tdL.score == maxScore)
    {
        numberOfWinners++;
        strcat(winnerName, " ");
        strcat(winnerName, tdL.nameOfClient);
    }

    lock = 0;
    while (!gameOver)
        ;

    if (numberOfWinners == 1)
    {
        strcat(msg, "\nCastigatorul este jucatorul cu numele : ");
        strcat(msg, winnerName);
    }
    else if (numberOfWinners > 1)
    {
        strcat(msg, "\nCastigatorii sunt jucatorii cu numele : ");
        strcat(msg, winnerName);
    }

    write(tdL.client, msg, 256);
    bzero(msg, 256);

    strcat(msg, "\nPunctajul maxim al jocului este egal cu : ");
    toString(score, maxScore);
    strcat(msg, score);
    write(tdL.client, msg, 256);
    bzero(msg, 256);
}

/**
 *  Function: timer
 *  -----------------------------
 *  writes data to client within a given time frame
 * 
 *  timeQuestion_t : the time when question i was sent
 *  currentQuestion : the number of the current question
 *  gameOver : becomes true when all the questions were sent and answered
*/
void timer()
{
    timeQuestion_t = time(NULL);

    for (int i = 0; i <= NR_QUESTIONS; i++)
    {
        while (timeQuestion_t > time(NULL) - SECONDS_PER_MESSAGE)
            ;

        currentQuestion++;
        timeQuestion_t = time(NULL);
    }

    timeQuestion_t = time(NULL);
    while (timeQuestion_t > time(NULL) - SECONDS_PER_MESSAGE)
        ;

    gameOver = true;
}

int main()
{
    int sd;

    /* crearea unui socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    /* pregatirea structurilor de date */
    struct sockaddr_in server;
    struct sockaddr_in from;

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    /* umplem structura folosita de server */
    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    /* utilizam un port utilizator */
    server.sin_port = htons(PORT);

    /* atasam socketul */
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen(sd, numberOfClients) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    printf("[server]Asteptam la portul %d...\n", PORT);
    fflush(stdout);

    int i = 0;

    startAccept_t = time(NULL);

    int flags = fcntl(sd, F_GETFL, 0);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK); // sets socket to nonblocking

    /* servim in mod iterativ clientii */
    while (1)
    {
        int client;
        int length = sizeof(from);

        /* nu mai pot fi acceptati clienti */
        if (startAccept_t < time(NULL) - 5)
        {
            startGame_t = time(NULL);
            printf("Nu mai pot fi acceptati clienti.\n");
            fflush(stdout);
            /* inchidem ultima conexiune creata */

            break;
        }
        else
        {
            /* acceptam un client */
            client = accept(sd, (struct sockaddr *)&from, &length);

            if (client < 1)
            {
                if (errno == EWOULDBLOCK) 
                {
                    continue;
                }
                else
                {
                    perror("Eroare la acceptarea conexiunii.");
                    continue;
                }
            }

            td = (struct thData *)malloc(sizeof(struct thData));
            strcpy(td->nameOfClient, "");
            td->idThread = i++;
            td->client = client;
            td->score = 0;

            pthread_create(&th[i], NULL, &treat, td);
        }
    }

    printf("A inceput jocul\n\n");
    fflush(stdout);

    timer();

    char msg[100];

    read(0, msg, 256);
}