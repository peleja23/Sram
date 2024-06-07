#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 5555
#define MAX_BUFFER 65536  // Podemos ter de aumentar se necessário
#define DIGIT_IMAGE_SIZE 4096  // Define an appropriate size for the image data

typedef struct {
    int A;      // Campo de controle
    int F;      // Campo de precisão
    char digitImages[12][DIGIT_IMAGE_SIZE];  // Array to hold the image data for the digits
} PDU;

void saveImage(const char* filename, char* buffer, int bufferSize) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening image file");
        exit(1);
    }
    fwrite(buffer, 1, bufferSize, file);
    fclose(file);
}

void receiveData(){
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server, client;
    PDU pdu;
    int clientLen = sizeof(client);

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return;
    }

    while(1) {
        recvfrom(sock, (char*)&pdu, sizeof(PDU), 0, (struct sockaddr *)&client, &clientLen);
        printf("A: %d, F: %d \n", pdu.A, pdu.F);
        // Save each digit image to disk
        for (int i = 0; i < 9 + pdu.F ; i++) {
            char filename[20];
            sprintf(filename, "digit_%d.png", i);
            saveImage(filename, pdu.digitImages[i], DIGIT_IMAGE_SIZE);
        }
    }

    closesocket(sock);
    WSACleanup();
}

int main(int argc, char* argv[]) {
    receiveData();
}
