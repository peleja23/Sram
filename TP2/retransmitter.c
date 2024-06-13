#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define RECEIVE_PORT 5555
#define SEND_PORT 5556
#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145

typedef struct {
    int A;
    int F;
    int Framecount;  
    char digitImages[16][DIGIT_IMAGE_SIZE];
} PDU;

int N, P, M;
time_t lastIgnoreTime;

pthread_mutex_t lock;
pthread_cond_t cond;

typedef struct {
    PDU* pdus;
    int size;
    int capacity;
} PDUBuffer;

PDUBuffer pduBuffer;
int bufferReady = 0;

void initPDUBuffer(PDUBuffer* buffer, int initialCapacity) {
    buffer->pdus = (PDU*)malloc(sizeof(PDU) * initialCapacity);
    buffer->size = 0;
    buffer->capacity = initialCapacity;
}

void addPDUToBuffer(PDUBuffer* buffer, PDU* pdu) {
    if (buffer->size >= buffer->capacity) {
        buffer->capacity *= 2;
        buffer->pdus = (PDU*)realloc(buffer->pdus, sizeof(PDU) * buffer->capacity);
    }
    buffer->pdus[buffer->size++] = *pdu;
}

void freePDUBuffer(PDUBuffer* buffer) {
    free(buffer->pdus);
}

void* receive_data(void* arg) {
    int recvSocket;
    struct sockaddr_in recvAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    recvSocket = socket(AF_INET, SOCK_DGRAM, 0);
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_addr.s_addr = INADDR_ANY;
    recvAddr.sin_port = htons(RECEIVE_PORT);

    bind(recvSocket, (struct sockaddr *)&recvAddr, sizeof(recvAddr));

    while (1) {
        PDU pdu;
        printf("[Receiver] Waiting for data...\n");

        recvfrom(recvSocket, &pdu, sizeof(PDU), 0, (struct sockaddr *)&clientAddr, &clientLen);
        pthread_mutex_lock(&lock);
        addPDUToBuffer(&pduBuffer, &pdu);
        bufferReady = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }

    close(recvSocket);
    return NULL;
}

void* send_data(void* arg) {
    int sendSocket;
    struct sockaddr_in sendAddr;
    int frameCount = 0;

    sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    sendAddr.sin_family = AF_INET;
    sendAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sendAddr.sin_port = htons(SEND_PORT);

    while (1) {
        pthread_mutex_lock(&lock);
        while (!bufferReady) {
            pthread_cond_wait(&cond, &lock);
        }

        // Ignores a PDU every M seconds
        time_t currentIgnoreTime = time(NULL);
        if (difftime(currentIgnoreTime, lastIgnoreTime) >= M) {
            lastIgnoreTime = currentIgnoreTime;
            bufferReady = 0;
            printf("[Sender] Ignoring PDU due to time condition\n");
            pthread_mutex_unlock(&lock);
            continue;
        }

        for (int i = 0; i < pduBuffer.size; ++i) {
            sendto(sendSocket, &pduBuffer.pdus[i], sizeof(PDU), 0, (struct sockaddr *)&sendAddr, sizeof(sendAddr));
        }
        pduBuffer.size = 0; // Clear the buffer after sending

        frameCount++;

        bufferReady = 0;
        pthread_mutex_unlock(&lock);

        if (frameCount >= N) {
            printf("[Sender] Sleeping for %d seconds...\n", P);
            sleep(P);
            frameCount = 0;
        }
    }

    close(sendSocket);
    return NULL;
}

int main(int argc, char* argv[]) {
    M = 15;
    N = 10;
    P = 5;
    if (argc > 1) {
        M = atoi(argv[1]);
    }
    if (argc > 2) {
        N = atoi(argv[2]);
    }
    if (argc > 3) {
        P = atoi(argv[3]);
    }

    lastIgnoreTime = time(NULL);

    pthread_t recvThread, sendThread;
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    initPDUBuffer(&pduBuffer, 10);

    int recvThreadStatus = pthread_create(&recvThread, NULL, receive_data, NULL);

    int sendThreadStatus = pthread_create(&sendThread, NULL, send_data, NULL);

    pthread_join(recvThread, NULL);
    pthread_join(sendThread, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    freePDUBuffer(&pduBuffer);

    return 0;
}
