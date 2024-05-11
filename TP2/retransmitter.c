#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8080
#define MAX_BUFFER 1024  // podemos ter de aumentar

void receiveData(){
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server;
    char server_reply[MAX_BUFFER];

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connect failed. Error\n");
        return;
    }

    while(1) {
        int len = recv(sock, server_reply, MAX_BUFFER, 0);
        if (len > 0) {
            server_reply[len] = '\0';  // Null-terminate the string for safety

            int A, F;
            memcpy(&A, server_reply, sizeof(int));
            memcpy(&F, server_reply + sizeof(int), sizeof(int));
            char* timeString = server_reply + 2 * sizeof(int);
            printf("A: %d, F: %d, Time: %s\n", A, F, timeString);
        } else if (len == 0) {
            printf("Connection closed\n");
            break;
        } else {
            printf("recv failed: %d\n", WSAGetLastError());
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
}

//TODO sendData

int main(int argc, char* argv[]) {
    receiveData();
} 