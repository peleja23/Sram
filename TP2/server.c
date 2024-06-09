#include <winsock2.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>

#define PORT 5555
#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145  

typedef struct {
    int A;     
    int F;     
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

void getHighPrecisionTime(double* seconds) {
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    *seconds = (double)counter.QuadPart / frequency.QuadPart;
}

void generateTimeString(int F, char* buffer) {
    SYSTEMTIME st;
    GetSystemTime(&st);
    
    double highPrecisionSeconds;
    getHighPrecisionTime(&highPrecisionSeconds);
    
    int microseconds = (int)((highPrecisionSeconds - (int)highPrecisionSeconds) * 1e6);

    switch (F) {
        case 0:
            sprintf(buffer, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
            break;
        case 1:
            sprintf(buffer, "%02d:%02d:%02d:%01d", st.wHour, st.wMinute, st.wSecond, microseconds / 100000);
            break;
        case 2:
            sprintf(buffer, "%02d:%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond, microseconds / 10000);
            break;
        case 3:
            sprintf(buffer, "%02d:%02d:%02d:%03d", st.wHour, st.wMinute, st.wSecond, microseconds / 1000);
            break;
        case 4:
            sprintf(buffer, "%02d:%02d:%02d:%04d", st.wHour, st.wMinute, st.wSecond, microseconds / 100);
            break;
        case 5:
            sprintf(buffer, "%02d:%02d:%02d:%05d", st.wHour, st.wMinute, st.wSecond, microseconds / 10);
            break;
        case 6:
            sprintf(buffer, "%02d:%02d:%02d:%06d", st.wHour, st.wMinute, st.wSecond, microseconds);
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

void preparePDU(PDU* pdu, int F, int A) {
    pdu->A = A;
    pdu->F = F;
    char timestamp[MAX_BUFFER];
    generateTimeString(F, timestamp);
    printf("%s\n", timestamp);
    int imageIndex = 0;
    for (int i = 0; i < strlen(timestamp) && imageIndex <= (9 + F); i++) {
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

void sendData(int F, int A) {
    WSADATA wsa;
    SOCKET udpSocket;
    struct sockaddr_in server;
    PDU pdu;
    WSAStartup(MAKEWORD(2,2), &wsa);
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);

    char previousTimestamp[MAX_BUFFER] = {0};
    char currentTimestamp[MAX_BUFFER];

    while (1) {
        generateTimeString(F, currentTimestamp);
        if (strcmp(previousTimestamp, currentTimestamp) != 0) {
            preparePDU(&pdu, F, A); 
            sendto(udpSocket, (const char*)&pdu, sizeof(PDU), 0, (struct sockaddr *)&server, sizeof(server));
            strcpy(previousTimestamp, currentTimestamp);
        }
    }
    closesocket(udpSocket);
    WSACleanup();
}

int main(int argc, char* argv[]) {
    preloadImages();
    int F = 0;
    int A = 2; 
    if (argc > 1) {
        F = atoi(argv[1]);
    }
    if (argc > 2) {
        A = atoi(argv[2]);
    }
    sendData(F, A);
}
