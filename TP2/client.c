#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <pthread.h>

#define PORT 5556
#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145

typedef struct {
    int A;
    int F;
    int Framecount;
    char digitImages[16][DIGIT_IMAGE_SIZE];
} PDU;

PDU *buffer;
int bufferSize;
int head = 0;
int tail = 0;
int count = 0;
int frameInterval = 1000; // Intervalo entre frames em microsegundos
pthread_mutex_t lock;
pthread_cond_t notEmpty;

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

void initBuffer(int size) {
    buffer = (PDU *)malloc(size * sizeof(PDU));
    bufferSize = size;
    head = 0;
    tail = 0;
    count = 0;
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&notEmpty, NULL);
}

void freeBuffer() {
    free(buffer);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&notEmpty);
}

void addToBuffer(PDU *pdu) {
    pthread_mutex_lock(&lock);
    if (count < bufferSize) {
        buffer[head] = *pdu;
        head = (head + 1) % bufferSize;
        count++;
    } else {
        printf("Buffer cheio, limpando buffer\n");
        head = 0;
        tail = 0;
        count = 0;
        buffer[head] = *pdu;
        head = (head + 1) % bufferSize;
        count = 1;
    }
    pthread_cond_signal(&notEmpty);
    pthread_mutex_unlock(&lock);
}

PDU* getFromBuffer() {
    PDU *pdu = NULL;
    pthread_mutex_lock(&lock);
    while (count == 0) {
        pthread_cond_wait(&notEmpty, &lock);
    }
    pdu = &buffer[tail];
    tail = (tail + 1) % bufferSize;
    count--;
    pthread_mutex_unlock(&lock);
    return pdu;
}

void* receivePDU(void *arg) {
    int udpSocket;
    struct sockaddr_in clientAddr;
    PDU pdu;

    // Cria o socket UDP
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        perror("Erro ao criar socket");
        pthread_exit(NULL);
    }

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(PORT);

    if (bind(udpSocket, (const struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
        perror("Erro ao fazer bind do socket");
        close(udpSocket);
        pthread_exit(NULL);
    }

    while (1) {
        int n = recvfrom(udpSocket, &pdu, sizeof(PDU), 0, NULL, NULL);
        if (n < 0) {
            perror("Erro ao receber dados");
            break;
        }

        if (n != sizeof(PDU)) {
            printf("Tamanho do PDU recebido incorreto: esperado %lu, recebido %d\n", sizeof(PDU), n);
            continue;
        }

        addToBuffer(&pdu);
    }

    close(udpSocket);
    pthread_exit(NULL);
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
    struct timespec lastFrameTime;
    clock_gettime(CLOCK_REALTIME, &lastFrameTime);
    while (running) {
        struct timespec currentTime;
        clock_gettime(CLOCK_REALTIME, &currentTime);
        long elapsedTime = (currentTime.tv_sec - lastFrameTime.tv_sec) * 1000000 + (currentTime.tv_nsec - lastFrameTime.tv_nsec) / 1000;

        if (elapsedTime >= frameInterval) {
            PDU *frame = getFromBuffer();
            if (frame != NULL) {
                displayTimeFromPDU(frame, renderer, textures);
            }
            lastFrameTime = currentTime;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
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

    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        perror("Erro ao criar socket");
        return 1;
    }

    struct sockaddr_in clientAddr;
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
    int recv_len = recvfrom(udpSocket, &initialPDU, sizeof(PDU), 0, NULL, NULL);
    if (recv_len < 0) {
        perror("Erro ao receber dados iniciais");
        close(udpSocket);
        return 1;
    }
    close(udpSocket);

    printf("Parâmetros recebidos: F=%d, A=%d\n", initialPDU.F, initialPDU.A);
    bufferSize = pow(10, initialPDU.F) * initialPDU.A;
    printf("Buffer Size: %d\n", bufferSize);
    initBuffer(bufferSize);

    // Define o intervalo entre frames com base no valor de F
    frameInterval = 1000000 / pow(10, initialPDU.F); // Em microsegundos
    printf("Frame Interval: %d microseconds\n", frameInterval);

    pthread_create(&receiverThread, NULL, receivePDU, NULL);
    pthread_create(&displayThread, NULL, displayClock, NULL);

    pthread_join(receiverThread, NULL);
    pthread_join(displayThread, NULL);

    freeBuffer();

    IMG_Quit();
    SDL_Quit();

    return 0;
}
