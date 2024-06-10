#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define RECEIVE_PORT 5555
#define SEND_PORT 5556
#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145

typedef struct {
    int A;
    int F;
    char digitImages[16][DIGIT_IMAGE_SIZE];
} PDU;

int N, P, M;
time_t lastIgnoreTime;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void reencaminhar() {
    WSADATA wsa;
    SOCKET recvSocket, sendSocket;
    struct sockaddr_in recvAddr, sendAddr, clientAddr;
    int recvLen, clientLen = sizeof(clientAddr);
    PDU pdu;
    int frameCount = 0;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error("WSAStartup failed");
    }

    // Configuração do socket de recebimento
    recvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (recvSocket == INVALID_SOCKET) {
        error("Erro ao criar socket de recebimento");
    }
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_addr.s_addr = INADDR_ANY;
    recvAddr.sin_port = htons(RECEIVE_PORT);

    if (bind(recvSocket, (struct sockaddr *)&recvAddr, sizeof(recvAddr)) == SOCKET_ERROR) {
        error("Erro ao fazer bind do socket de recebimento");
    }

    // Configuração do socket de envio
    sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sendSocket == INVALID_SOCKET) {
        error("Erro ao criar socket de envio");
    }
    sendAddr.sin_family = AF_INET;
    sendAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sendAddr.sin_port = htons(SEND_PORT);

    while (1) {
        printf("Waiting for data...\n"); // Debugging print

        // Recebe PDU do servidor
        recvLen = recvfrom(recvSocket, (char *)&pdu, sizeof(PDU), 0, (struct sockaddr *)&clientAddr, &clientLen);
        if (recvLen == SOCKET_ERROR) {
            error("Erro ao receber dados");
        }

        printf("Received PDU: A=%d, F=%d\n", pdu.A, pdu.F); // Debugging print

        frameCount++;

        // Ignora um PDU a cada M segundos
        time_t currentIgnoreTime = time(NULL);
        if (difftime(currentIgnoreTime, lastIgnoreTime) >= M) {
            lastIgnoreTime = currentIgnoreTime;
            printf("Ignoring PDU due to time condition\n"); // Debugging print
            continue; // Skip sending this PDU
        }

        // Envia o PDU para o cliente
        int sendLen = sendto(sendSocket, (const char *)&pdu, sizeof(PDU), 0, (struct sockaddr *)&sendAddr, sizeof(sendAddr));
        if (sendLen == SOCKET_ERROR) {
            error("Erro ao enviar dados");
        }

        printf("Sent PDU to client\n"); // Debugging print

        // Pausa a cada N frames
        if (frameCount >= N) {
            printf("Sleeping for %d seconds...\n", P); // Debugging print
            Sleep(P * 1000);
            frameCount = 0; // Reset the frame counter after sleeping
        }
    }

    closesocket(recvSocket);
    closesocket(sendSocket);
    WSACleanup();
}

int main(int argc, char* argv[]) {
    // Assign global variables
    M = 20;
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

    // Initialize lastIgnoreTime to the current time
    lastIgnoreTime = time(NULL);

    reencaminhar();

    return 0;
}
