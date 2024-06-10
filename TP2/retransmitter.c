#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
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
    int recvSocket, sendSocket;
    struct sockaddr_in recvAddr, sendAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    PDU pdu;
    int frameCount = 0;

    recvSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (recvSocket < 0) {
        error("Error creating receive socket");
    }
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_addr.s_addr = INADDR_ANY;
    recvAddr.sin_port = htons(RECEIVE_PORT);

    if (bind(recvSocket, (struct sockaddr *)&recvAddr, sizeof(recvAddr)) < 0) {
        error("Error binding receive socket");
    }

    sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sendSocket < 0) {
        error("Error creating send socket");
    }
    sendAddr.sin_family = AF_INET;
    sendAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sendAddr.sin_port = htons(SEND_PORT);

    while (1) {
        printf("Waiting for data...\n"); // Debugging print

        // Receives PDU from server
        int recvLen = recvfrom(recvSocket, &pdu, sizeof(PDU), 0, (struct sockaddr *)&clientAddr, &clientLen);
        if (recvLen < 0) {
            error("Error receiving data");
        }

        printf("Received PDU: A=%d, F=%d\n", pdu.A, pdu.F); // Debugging print

        frameCount++;

        // Ignores a PDU every M seconds
        time_t currentIgnoreTime = time(NULL);
        if (difftime(currentIgnoreTime, lastIgnoreTime) >= M) {
            lastIgnoreTime = currentIgnoreTime;
            printf("Ignoring PDU due to time condition\n"); // Debugging print
            continue; // Skip sending this PDU
        }

        // Sends the PDU to the client
        int sendLen = sendto(sendSocket, &pdu, sizeof(PDU), 0, (struct sockaddr *)&sendAddr, sizeof(sendAddr));
        if (sendLen < 0) {
            error("Error sending data");
        }

        printf("Sent PDU to client\n"); // Debugging print

        // Pauses every N frames
        if (frameCount >= N) {
            printf("Sleeping for %d seconds...\n", P); // Debugging print
            sleep(P);
            frameCount = 0; // Reset the frame counter after sleeping
        }
    }

    close(recvSocket);
    close(sendSocket);
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
