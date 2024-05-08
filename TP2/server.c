#include <winsock2.h>
#include <stdio.h>

void senData(){
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in server;
    char *message = "Ola";
    WSAStartup(MAKEWORD(2,2), &wsa);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    bind(serverSocket, (struct sockaddr *)&server, sizeof(server));

    listen(serverSocket, 1);

    clientSocket = accept(serverSocket, NULL, NULL);

    send(clientSocket, message, strlen(message), 0);

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
}
int main(int argc, char* argv[]) {
    senData();
}