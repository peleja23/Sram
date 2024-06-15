#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>

#define SERVER_PORT 5555
#define CLIENT_PORT 5556
#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145
#define BUFFER_SIZE 20000  // Adjust based on expected buffer needs

typedef struct {
    int A;
    int F;
    int Framecount;
    char digitImages[16][DIGIT_IMAGE_SIZE];
} PDU;

PDU buffer[BUFFER_SIZE];
int bufferHead = 0;
int bufferTail = 0;
int N;  // Number of frames after which to pause
int P;  // Pause duration in seconds
int M;  // Interval in seconds to skip a PDU

pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bufferNotEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t bufferNotFull = PTHREAD_COND_INITIALIZER;

void* receiveFromServer(void* arg) {
    int sock;
    struct sockaddr_in server, client;
    PDU pdu;
    socklen_t clientLen = sizeof(client);
    time_t lastSkipTime = time(NULL);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(SERVER_PORT);

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    while (1) {
        recvfrom(sock, &pdu, sizeof(PDU), 0, (struct sockaddr *)&client, &clientLen);

        time_t currentTime = time(NULL);
        if (difftime(currentTime, lastSkipTime) >= M) {
            printf("Skipping PDU due to M seconds interval...\n");
            lastSkipTime = currentTime;
            continue;  // Skip this PDU
        }

        pthread_mutex_lock(&bufferMutex);
        while ((bufferTail + 1) % BUFFER_SIZE == bufferHead) {
            pthread_cond_wait(&bufferNotFull, &bufferMutex);
        }
        
        buffer[bufferTail] = pdu;
        bufferTail = (bufferTail + 1) % BUFFER_SIZE;
        pthread_cond_signal(&bufferNotEmpty);
        pthread_mutex_unlock(&bufferMutex);
    }

    close(sock);
    return NULL;
}

void* retransmitToClient(void* arg) {
    int sock;
    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);
    int frameCount = 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    client.sin_family = AF_INET;
    client.sin_addr.s_addr = inet_addr("127.0.0.1");
    client.sin_port = htons(CLIENT_PORT);

    while (1) {
        pthread_mutex_lock(&bufferMutex);
        while (bufferHead == bufferTail) {
            pthread_cond_wait(&bufferNotEmpty, &bufferMutex);
        }

        PDU pdu = buffer[bufferHead];
        bufferHead = (bufferHead + 1) % BUFFER_SIZE;
        pthread_cond_signal(&bufferNotFull);
        pthread_mutex_unlock(&bufferMutex);

        frameCount++;
        printf("Retransmitting Frame Number: %d -> A: %d, F: %d, Framecount: %d\n",
               frameCount, pdu.A, pdu.F, pdu.Framecount);

        if (frameCount % N == 0) {
            printf("Pausing for %d seconds...\n", P);
            sleep(P);
        }

        sendto(sock, &pdu, sizeof(PDU), 0, (struct sockaddr *)&client, clientLen);
    }

    close(sock);
    return NULL;
}

int main(int argc, char* argv[]) {
    N = 100;  
    P = 3;   
    M = 8; 
    if (argc > 1) {
        N = atoi(argv[1]);
    }
    if (argc > 2) {
        P = atoi(argv[2]);
    }
    if (argc > 3) {
        M = atoi(argv[3]);
    }

    printf("Starting retransmitter with N=%d, P=%d, M=%d\n", N, P, M);

    pthread_t receiverThread, retransmitterThread;
    pthread_create(&receiverThread, NULL, receiveFromServer, NULL);
    pthread_create(&retransmitterThread, NULL, retransmitToClient, NULL);

    pthread_join(receiverThread, NULL);
    pthread_join(retransmitterThread, NULL);

    return 0;
}
