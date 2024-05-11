#include <winsock2.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>

#define PORT 8080
#define MAX_BUFFER 1024 // // temos de aumentar por causa das imagens

void sendData(int F, int A) {
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in server;
    char buffer[100];  
    time_t rawtime;
    struct tm * timeinfo;

    int interval = pow(10, -F) * 1000; // Intervalo em milissegundos
    char timeFormat[50] = "%H:%M:%S";

    WSAStartup(MAKEWORD(2,2), &wsa);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
    bind(serverSocket, (struct sockaddr *)&server, sizeof(server));
    listen(serverSocket, 1);
    clientSocket = accept(serverSocket, NULL, NULL);

    while(1) {
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), timeFormat, timeinfo);  // Isto ta mal dado que vai ter de se enviar as imagens msm

        int timeLength = strlen(buffer);
        int pduLength = sizeof(int) * 2 + timeLength + 1; 
        char* pdu = (char*) malloc(pduLength);
        int offset = 0;

        memcpy(pdu + offset, &A, sizeof(int));
        offset += sizeof(int);
        memcpy(pdu + offset, &F, sizeof(int));
        offset += sizeof(int);
        // Copia a string da hora para o PDU
        memcpy(pdu + offset, buffer, timeLength + 1);        
        send(clientSocket, pdu, pduLength, 0);
        free(pdu);
        Sleep(interval);
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
}

int main(int argc, char* argv[]) {
    //TODO: meter istio a ler de args
    int F = 1;
    int A = 2; 
    sendData(F, A);
}