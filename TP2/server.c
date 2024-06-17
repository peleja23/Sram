#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145  

typedef struct {
    int A;     
    int F;  
    int Framecount;   
    char digitImages[16][DIGIT_IMAGE_SIZE];  
} PDU;

char* digitFiles[] = {
    "digitos/zero.png", "digitos/um.png", "digitos/dois.png",
    "digitos/tres.png", "digitos/quatro.png", "digitos/cinco.png",
    "digitos/seis.png", "digitos/sete.png", "digitos/oito.png",
    "digitos/nove.png"
};
const char* separatorFile = "digitos/separador.png";

char digitImageBuffers[10][DIGIT_IMAGE_SIZE];
char separatorImageBuffer[DIGIT_IMAGE_SIZE];

PDU pdu;
long totalFramesSent = 0;
struct timespec startTime, endTime;
int keepRunning = 1;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void generateTimeString(int F, char* buffer) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm *tm_info = localtime(&ts.tv_sec);

    int microseconds = ts.tv_nsec / 1000;

    switch (F) {
        case 0:
            sprintf(buffer, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            break;
        case 1:
            sprintf(buffer, "%02d:%02d:%02d:%01d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 100000);
            break;
        case 2:
            sprintf(buffer, "%02d:%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 10000);
            break;
        case 3:
            sprintf(buffer, "%02d:%02d:%02d:%03d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 1000);
            break;
        case 4:
            sprintf(buffer, "%02d:%02d:%02d:%04d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 100);
            break;
        case 5:
            sprintf(buffer, "%02d:%02d:%02d:%05d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 10);
            break;
        case 6:
            sprintf(buffer, "%02d:%02d:%02d:%06d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds);
            break;
    }
}

void loadImage(const char* filename, char* buffer, int bufferSize) {
    FILE* file = fopen(filename, "rb");
    size_t bytesRead = fread(buffer, 1, bufferSize, file);
    fclose(file);
}

void preloadImages() {
    for (int i = 0; i < 10; i++) {
        loadImage(digitFiles[i], digitImageBuffers[i], DIGIT_IMAGE_SIZE);
    }
    loadImage(separatorFile, separatorImageBuffer, DIGIT_IMAGE_SIZE);
}

void preparePDU(PDU* pdu, int F, int A, int Framecount) {
    pdu->A = A;
    pdu->F = F;
    pdu->Framecount = Framecount;
    char timestamp[MAX_BUFFER];
    generateTimeString(F, timestamp);
    printf("%s\n", timestamp);
    int imageIndex = 0;
    for (int i = 0; i < strlen(timestamp); i++) {
        if (timestamp[i] == ':') {
            memcpy(pdu->digitImages[imageIndex], separatorImageBuffer, DIGIT_IMAGE_SIZE);
        } else {
            int digit = timestamp[i] - '0';
            if (digit >= 0 && digit <= 9) {
                memcpy(pdu->digitImages[imageIndex], digitImageBuffers[digit], DIGIT_IMAGE_SIZE);
            }
        }
        imageIndex++;
    }
}

void sendData(int F, int A, int Framecount, const char* ip, int port) {
    int udpSocket;
    struct sockaddr_in server;

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);

    char previousTimestamp[MAX_BUFFER] = {0};
    char currentTimestamp[MAX_BUFFER];

    clock_gettime(CLOCK_REALTIME, &startTime);

    while (1) {
        pthread_mutex_lock(&lock);
        if (!keepRunning) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);

        generateTimeString(F, currentTimestamp);
        if (strcmp(previousTimestamp, currentTimestamp) != 0) {
            Framecount = Framecount + 1;
            printf("Frame: %d ->", Framecount);
            preparePDU(&pdu, F, A, Framecount); 
            sendto(udpSocket, &pdu, sizeof(PDU), 0, (struct sockaddr *)&server, sizeof(server));
            strcpy(previousTimestamp, currentTimestamp);
        }
    }

    clock_gettime(CLOCK_REALTIME, &endTime);
    close(udpSocket);
}

void readServerConfig(const char* filename, char* ip, int* port) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open server config file");
        exit(EXIT_FAILURE);
    }
    fscanf(file, "%d", port);
    fscanf(file, "%s", ip);
    fclose(file);
}

void printStatistics(PDU* pdu) {
    double totalTime = (endTime.tv_sec - startTime.tv_sec);
    double frameRate = pdu->Framecount / totalTime;

    printf("\n--- Estatísticas ---\n");
    printf("Total de frames enviadas: %d\n", pdu->Framecount);
    printf("Tempo de execução: %.2f segundos\n", totalTime);
    printf("Ritmo de informação: %.2f frames por segundo\n", frameRate);
}

void* listenForExit(void* arg) {
    char input[10];
    while (1) {
        printf("Digite 'Q' para terminar a execução: ");
        scanf("%s", input);
        if (strcmp(input, "Q") == 0) {
            pthread_mutex_lock(&lock);
            keepRunning = 0;
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&lock);
            break;
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    preloadImages();
    int F = 0;
    int A = 2;
    int Framecount = 0; 
    char ip[INET_ADDRSTRLEN];
    int port;

    readServerConfig("server.txt", ip, &port);

    if (argc > 1) {
        F = atoi(argv[1]);
    }
    if (argc > 2) {
        A = atoi(argv[2]);
    }

    pthread_t exitThread;
    pthread_create(&exitThread, NULL, listenForExit, NULL);

    sendData(F, A, Framecount, ip, port);

    pthread_join(exitThread, NULL);
    printStatistics(&pdu);

    return 0;
}
