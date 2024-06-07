#include <winsock2.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define PORT 5555
#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 4096  

typedef struct {
    int A;     
    int F;     
    char digitImages[8][DIGIT_IMAGE_SIZE];  
} PDU;

char* digitFiles[] = {
    "digitos/zero.png", "digitos/um.png", "digitos/dois.png",
    "digitos/tres.png", "digitos/quatro.png", "digitos/cinco.png",
    "digitos/seis.png", "digitos/sete.png", "digitos/oito.png",
    "digitos/nove.png"
};
const char* separatorFile = "digitos/separador.png";

void generateTimeString(int F, char* buffer) {
    struct timespec ts;
    struct tm* timeinfo;

    clock_gettime(CLOCK_REALTIME, &ts);
    timeinfo = localtime(&ts.tv_sec);

    int ms = ts.tv_nsec / 1000000;      // Milissegundos
    int ds = ms / 100;                  // Décimos de segundo
    int cs = ms / 10;                   // Centésimos de segundo
    int us = ts.tv_nsec / 1000;         // Microssegundos
    int ns = ts.tv_nsec;                // Nanossegundos

    switch (F) {
        case 0:
            sprintf(buffer, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            break;
        case 1:
            sprintf(buffer, "%02d:%02d:%02d:%01d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ds % 10);
            break;
        case 2:
            sprintf(buffer, "%02d:%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, cs % 100);
            break;
        case 3:
            sprintf(buffer, "%02d:%02d:%02d:%03d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ms);
            break;
        case 4:
            sprintf(buffer, "%02d:%02d:%02d:%03d%01d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ms, us % 10);
            break;
        case 5:
            sprintf(buffer, "%02d:%02d:%02d:%03d%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ms, us % 100);
            break;
        case 6:
            sprintf(buffer, "%02d:%02d:%02d:%03d%03d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ms, us);
            break;
        case 7:
            sprintf(buffer, "%02d:%02d:%02d:%03d%03d%03d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ms, us, ns % 1000);
            break;
    }
}

void loadImage(const char* filename, char* buffer, int bufferSize) {
    FILE* file = fopen(filename, "rb");
    size_t bytesRead = fread(buffer, 1, bufferSize, file);
    fclose(file);
}

void preparePDU(PDU* pdu, int F, int A) {
    pdu->A = A;
    pdu->F = F;
    char timestamp[MAX_BUFFER];
    generateTimeString(F, timestamp);
    int imageIndex = 0;
    for (int i = 0; i < strlen(timestamp) && imageIndex < 8 + F; i++) {
        if (timestamp[i] == ':') {
            loadImage(separatorFile, pdu->digitImages[imageIndex], DIGIT_IMAGE_SIZE);
        } else {
            int digit = timestamp[i] - '0';
            if (digit >= 0 && digit <= 9) {
                loadImage(digitFiles[digit], pdu->digitImages[imageIndex], DIGIT_IMAGE_SIZE);
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
    int interval = pow(10, -F) * 1000000;
    WSAStartup(MAKEWORD(2,2), &wsa);
    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);

    while(1) {
        preparePDU(&pdu, F, A); 
        printf("%i \n",&pdu);
        sendto(udpSocket, (const char*)&pdu, sizeof(PDU), 0, (struct sockaddr *)&server, sizeof(server));      
        struct timespec ts;
        ts.tv_sec = interval / 1000000;
        ts.tv_nsec = (interval % 1000000) * 1000;
        nanosleep(&ts, NULL);
    }

    closesocket(udpSocket);
    WSACleanup();
}

int main(int argc, char* argv[]) {
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
