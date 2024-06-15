#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define PORT 5556
#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145

typedef struct {
    int A;
    int F;
    int Framecount;
    char digitImages[16][DIGIT_IMAGE_SIZE];
} PDU;

SDL_Texture* loadTextureFromMemory(SDL_Renderer *renderer, const char *data, int size) {
    if (size == 0 || data[0] == '\0') {
        return NULL; // Retorna NULL se os dados forem vazios
    }
    SDL_RWops *rw = SDL_RWFromConstMem(data, size);
    if (!rw) {
        printf("Erro ao criar RWops: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_Surface *surface = IMG_Load_RW(rw, 1);
    if (!surface) {
        printf("Erro ao carregar imagem da memória: %s\n", IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void displayTimeFromPDU(PDU *pdu, SDL_Renderer *renderer, SDL_Texture **textures) {
    SDL_Rect dst;
    dst.y = 80;
    dst.w = 40;
    dst.h = 40;

    for (int i = 0; i < 16; i++) {
        if (textures[i] != NULL) {
            SDL_DestroyTexture(textures[i]);
        }
        textures[i] = loadTextureFromMemory(renderer, pdu->digitImages[i], DIGIT_IMAGE_SIZE);
        if (textures[i] == NULL) {
            continue; // Continua se a textura for vazia ou nula
        }

        dst.x = 0 + i * 64;
        SDL_RenderCopy(renderer, textures[i], NULL, &dst);
    }
    SDL_RenderPresent(renderer);
}

int main() {
    int udpSocket;
    struct sockaddr_in clientAddr;
    PDU pdu;
    int bufferSize;
    int running = 1;

    // Inicializa SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Erro ao iniciar SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Inicializa SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("Erro ao iniciar SDL_image: %s\n", IMG_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Relógio Digital", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 200, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Erro ao criar janela: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *textures[16];
    memset(textures, 0, sizeof(textures));

    // Cria o socket UDP
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        perror("Erro ao criar socket");
        return 1;
    }

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(PORT);

    if (bind(udpSocket, (const struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
        perror("Erro ao fazer bind do socket");
        close(udpSocket);
        return 1;
    }

    // Recebe os parâmetros F e A
    int recv_len = recvfrom(udpSocket, &pdu, sizeof(PDU), 0, NULL, NULL);
    if (recv_len < 0) {
        perror("Erro ao receber dados iniciais");
        close(udpSocket);
        return 1;
    }
    printf("Parâmetros recebidos: F=%d, A=%d\n", pdu.F, pdu.A);
    bufferSize = pow(10,pdu.F)*pdu.A;
    printf("Buffer Size: %d ",bufferSize);

    while (running) {
        int n = recvfrom(udpSocket, &pdu, sizeof(PDU), 0, NULL, NULL);
        if (n < 0) {
            perror("Erro ao receber dados");
            break;
        }

        if (n != sizeof(PDU)) {
            printf("Tamanho do PDU recebido incorreto: esperado %lu, recebido %d\n", sizeof(PDU), n);
            continue;
        }

        displayTimeFromPDU(&pdu, renderer, textures);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
    }

    // Libera recursos
    for (int i = 0; i < 16; i++) {
        if (textures[i]) {
            SDL_DestroyTexture(textures[i]);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    close(udpSocket);
    return 0;
}
