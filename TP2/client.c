#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5556
#define MAX_BUFFER 65536  // We can increase if necessary
#define DIGIT_IMAGE_SIZE 2145   // Define an appropriate size for the image data

typedef struct {
    int A;      // Control field
    int F;      // Precision field
    char digitImages[16][DIGIT_IMAGE_SIZE];  // Array to hold the image data for the digits
} PDU;

void saveImage(const char* filename, char* buffer, int bufferSize) {
    FILE* file = fopen(filename, "wb");
    fwrite(buffer, 1, bufferSize, file);
    fclose(file);
}

void receiveData(){
    int sock;
    struct sockaddr_in server, client;
    PDU pdu;
    socklen_t clientLen = sizeof(client);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    while(1) {
        recvfrom(sock, &pdu, sizeof(PDU), 0, (struct sockaddr *)&client, &clientLen);
        printf("A: %d, F: %d \n", pdu.A, pdu.F);
        // Save each digit image to disk
        for (int i = 0; i < 9 + pdu.F; i++) {  // Ensure we do not exceed array bounds
            char filename[50];
            sprintf(filename, "digit_%d.png", i);
            saveImage(filename, pdu.digitImages[i], DIGIT_IMAGE_SIZE);
        }
    }

    close(sock);
}

int main(int argc, char* argv[]) {
    receiveData();
    return 0;
}
