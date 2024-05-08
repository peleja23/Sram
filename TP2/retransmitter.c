#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8080

void receiveData(){
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in server;
    char server_reply[200];

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    int len = recv(sock, server_reply, 200, 0);
    server_reply[len] = '\0'; // Null-terminate the string
    printf("Server: %s\n", server_reply);

    closesocket(sock);
    WSACleanup();
}
int main(int argc, char* argv[]) {
    receiveData();
}