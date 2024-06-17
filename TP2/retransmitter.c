#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>

#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145
#define BUFFER_SIZE 30000  // Adjust based on expected buffer needs

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
int serverPort;
int clientPort;
char serverIp[INET_ADDRSTRLEN];

// Variáveis globais para controle e estatísticas
int keepRunning = 1;
long skippedFrames = 0;
double totalPausedTime = 0.0;
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t statsMutex = PTHREAD_MUTEX_INITIALIZER;
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
    server.sin_port = htons(serverPort);

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    while (1) {
        pthread_mutex_lock(&statsMutex);
        if (!keepRunning) {
            pthread_mutex_unlock(&statsMutex);
            break;
        }
        pthread_mutex_unlock(&statsMutex);

        recvfrom(sock, &pdu, sizeof(PDU), 0, (struct sockaddr *)&client, &clientLen);

        time_t currentTime = time(NULL);
        if (difftime(currentTime, lastSkipTime) >= M) {
            printf("Skipping PDU due to M seconds interval...\n");
            lastSkipTime = currentTime;
            pthread_mutex_lock(&statsMutex);
            skippedFrames++;
            pthread_mutex_unlock(&statsMutex);
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
    client.sin_addr.s_addr = inet_addr(serverIp);
    client.sin_port = htons(clientPort);

    while (1) {
        pthread_mutex_lock(&statsMutex);
        if (!keepRunning) {
            pthread_mutex_unlock(&statsMutex);
            break;
        }
        pthread_mutex_unlock(&statsMutex);

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
            struct timespec startPause, endPause;
            clock_gettime(CLOCK_REALTIME, &startPause);
            sleep(P);
            clock_gettime(CLOCK_REALTIME, &endPause);
            pthread_mutex_lock(&statsMutex);
            totalPausedTime += (endPause.tv_sec - startPause.tv_sec) + (endPause.tv_nsec - startPause.tv_nsec) / 1e9;
            pthread_mutex_unlock(&statsMutex);
        }

        sendto(sock, &pdu, sizeof(PDU), 0, (struct sockaddr *)&client, clientLen);
    }

    close(sock);
    return NULL;
}

void readServerConfig(const char* filename, int* serverPort, int* clientPort, char* serverIp) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open server config file");
        exit(EXIT_FAILURE);
    }
    fscanf(file, "%d", serverPort);
    fscanf(file, "%d", clientPort);
    fscanf(file, "%s", serverIp);
    fclose(file);
}

void printStatistics() {
    pthread_mutex_lock(&statsMutex);
    printf("\n--- Estatísticas ---\n");
    printf("Total de frames ingnorados: %ld\n", skippedFrames);
    printf("Tempo total parado: %.2f segundos\n", totalPausedTime);
    pthread_mutex_unlock(&statsMutex);
}

void* listenForExit(void* arg) {
    char input[10];
    while (1) {
        printf("Digite 'Q' para terminar a execução: ");
        scanf("%s", input);
        if (strcmp(input, "Q") == 0) {
            pthread_mutex_lock(&statsMutex);
            keepRunning = 0;
            pthread_mutex_unlock(&statsMutex);
            pthread_cond_broadcast(&bufferNotEmpty);
            pthread_cond_broadcast(&bufferNotFull);
            break;
        }
    }
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

    readServerConfig("retransmitter.txt", &serverPort, &clientPort, serverIp);

    printf("Starting retransmitter with N=%d, P=%d, M=%d\n", N, P, M);

    pthread_t receiverThread, retransmitterThread, exitThread;
    pthread_create(&receiverThread, NULL, receiveFromServer, NULL);
    pthread_create(&retransmitterThread, NULL, retransmitToClient, NULL);
    pthread_create(&exitThread, NULL, listenForExit, NULL);

    pthread_join(exitThread, NULL);
    pthread_join(receiverThread, NULL);
    pthread_join(retransmitterThread, NULL);

    printStatistics();

    return 0;
}
