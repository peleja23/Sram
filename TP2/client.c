#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>

#define DIGIT_IMAGE_SIZE 2145

typedef struct {
    int A;
    int F;
    int Framecount;
    char digitImages[16][DIGIT_IMAGE_SIZE];
} PDU;

PDU *buffer;
int bufferSize;  // Nova variável para armazenar o tamanho do buffer
int bufferHead = 0;
int bufferTail = 0;
int frameInterval = 0; // Intervalo entre frames em microsegundos
int serverPort;
int receptionPort;
char serverIp[INET_ADDRSTRLEN];
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bufferNotEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t bufferNotFull = PTHREAD_COND_INITIALIZER;

SDL_Texture* loadTextureFromMemory(SDL_Renderer *renderer, const char *data, int size) {
    if (size == 0 || data[0] == '\0') {
        return NULL;
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
            continue;
        }
        dst.x = 0 + i * 64;
        SDL_RenderCopy(renderer, textures[i], NULL, &dst);
    }
    SDL_RenderPresent(renderer);
}

void* receivePDU(void* arg) {
    int udpSocket;
    struct sockaddr_in serverAddr;
    PDU pdu;

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        perror("Erro ao criar socket");
        pthread_exit(NULL);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(receptionPort);

    if (bind(udpSocket, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Erro ao fazer bind do socket");
        close(udpSocket);
        pthread_exit(NULL);
    }

    while (1) {
        recvfrom(udpSocket, &pdu, sizeof(PDU), 0, NULL, NULL);

        pthread_mutex_lock(&bufferMutex);
        while ((bufferTail + 1) % bufferSize == bufferHead) {
            pthread_cond_wait(&bufferNotFull, &bufferMutex);
        }
        
        buffer[bufferTail] = pdu;
        bufferTail = (bufferTail + 1) % bufferSize;
        pthread_cond_signal(&bufferNotEmpty);
        pthread_mutex_unlock(&bufferMutex);
    }

    close(udpSocket);
    return NULL;
}

void* displayClock(void *arg) {
    SDL_Window *window = SDL_CreateWindow("Relógio Digital", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 200, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Erro ao criar janela: %s\n", SDL_GetError());
        pthread_exit(NULL);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *textures[16];
    memset(textures, 0, sizeof(textures));

    int running = 1;
    while (running) {
        pthread_mutex_lock(&bufferMutex);
        while (bufferHead == bufferTail) {
            pthread_cond_wait(&bufferNotEmpty, &bufferMutex);
        }

        PDU frame = buffer[bufferHead];
        bufferHead = (bufferHead + 1) % bufferSize;
        pthread_cond_signal(&bufferNotFull);
        pthread_mutex_unlock(&bufferMutex);
        usleep(frameInterval);
        displayTimeFromPDU(&frame, renderer, textures);
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        SDL_Delay(10);  // Pequeno atraso para permitir que o sistema operacional processe outros eventos
    }

    for (int i = 0; i < 16; i++) {
        if (textures[i]) {
            SDL_DestroyTexture(textures[i]);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    pthread_exit(NULL);
}

void readReceptionPort(const char* filename, int* port) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }
    fscanf(file, "%d", port);
    fclose(file);
}

int main() {
    PDU initialPDU;
    pthread_t receiverThread, displayThread;

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

    // Lê a porta de recepção do ficheiro
    readReceptionPort("client.txt", &receptionPort);
    printf("Porta de recepção lida do ficheiro: %d\n", receptionPort);

    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        perror("Erro ao criar socket");
        return 1;
    }

    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(receptionPort);

    if (bind(udpSocket, (const struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
        perror("Erro ao fazer bind do socket");
        close(udpSocket);
        return 1;
    }

    // Recebe os parâmetros F e A
    int recv_len = recvfrom(udpSocket, &initialPDU, sizeof(PDU), 0, NULL, NULL);
    if (recv_len < 0) {
        perror("Erro ao receber dados iniciais");
        close(udpSocket);
        return 1;
    }
    close(udpSocket);

    printf("Parâmetros recebidos: F=%d, A=%d\n", initialPDU.F, initialPDU.A);
    
    // Define o tamanho do buffer e o intervalo entre frames com base nos valores de F e A
    bufferSize = pow(10, initialPDU.F) * initialPDU.A;
    buffer = malloc(bufferSize * sizeof(PDU));
    if (buffer == NULL) {
        perror("Erro ao alocar memória para o buffer");
        return 1;
    }

    frameInterval = 1000000 / pow(10, initialPDU.F); // Em microsegundos
    printf("Frame Interval: %d microseconds\n", frameInterval);

    pthread_create(&receiverThread, NULL, receivePDU, NULL);
    pthread_create(&displayThread, NULL, displayClock, NULL);

    pthread_join(receiverThread, NULL);
    pthread_join(displayThread, NULL);


    IMG_Quit();
    SDL_Quit();

    return 0;
}
