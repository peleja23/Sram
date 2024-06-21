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
int bufferSize = 2;
int bufferHead = 0;
int bufferTail = 0;
int frameInterval = 0;
int bufferReset = 0;
int framesCounter = 0;
int skippedFrames = 0;
int receptionPort;
int firstFrameCount = 0;
int lastFrameCount = 0;
int keepRunning = 1;

pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t statsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bufferNotEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t bufferNotFull = PTHREAD_COND_INITIALIZER;

void updateBufferAndInterval(int A, int F) {
    pthread_mutex_lock(&bufferMutex);

    int newBufferSize = pow(10, F) * A;
    int newFrameInterval = 1000000 / pow(10, F);

    if (newBufferSize != bufferSize) {
        PDU *newBuffer = malloc(newBufferSize * sizeof(PDU));
        if (bufferHead <= bufferTail) {
            memcpy(newBuffer, buffer + bufferHead, (bufferTail - bufferHead) * sizeof(PDU));
        } else {
            int firstPartSize = bufferSize - bufferHead;
            memcpy(newBuffer, buffer + bufferHead, firstPartSize * sizeof(PDU));
            memcpy(newBuffer + firstPartSize, buffer, bufferTail * sizeof(PDU));
        }

        bufferSize = newBufferSize;
        bufferTail = (bufferTail - bufferHead + bufferSize) % bufferSize;
        bufferHead = 0;

        free(buffer);
        buffer = newBuffer;
    }

    frameInterval = newFrameInterval;

    pthread_mutex_unlock(&bufferMutex);
}

void* receivePDU(void* arg) {
    int udpSocket;
    struct sockaddr_in serverAddr;
    PDU pdu;

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
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
        pthread_mutex_lock(&statsMutex);
        if (!keepRunning) {
            pthread_mutex_unlock(&statsMutex);
            break;
        }
        pthread_mutex_unlock(&statsMutex);

        recvfrom(udpSocket, &pdu, sizeof(PDU), 0, NULL, NULL);
        framesCounter++;

        if (firstFrameCount == 0) {
            firstFrameCount = pdu.Framecount;
        }
        lastFrameCount = pdu.Framecount;

        if (pdu.A != bufferSize / pow(10, pdu.F) || 1000000 / pow(10, pdu.F) != frameInterval) {
            updateBufferAndInterval(pdu.A, pdu.F);
        }

        pthread_mutex_lock(&bufferMutex);
        if ((bufferTail + 1) % bufferSize == bufferHead) {
            int skipped = (bufferTail + bufferSize - bufferHead) % bufferSize;
            printf("Buffer full. Resynchronizing by discarding %d frames.\n", skipped);
            skippedFrames += skipped;
            bufferHead = 0;
            bufferTail = 0;
            bufferReset++;
        }

        buffer[bufferTail] = pdu;
        bufferTail = (bufferTail + 1) % bufferSize;
        pthread_cond_signal(&bufferNotEmpty);
        pthread_mutex_unlock(&bufferMutex);
    }

    close(udpSocket);
    return NULL;
}

SDL_Texture* loadTextureFromMemory(SDL_Renderer *renderer, const char *data, int size) {
    if (size == 0 || data[0] == '\0') {
        return NULL;
    }
    SDL_RWops *rw = SDL_RWFromConstMem(data, size);

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

void* displayClock(void *arg) {
    SDL_Window *window = SDL_CreateWindow("Relógio Digital", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1000, 200, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Erro ao criar janela: %s\n", SDL_GetError());
        pthread_exit(NULL);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *textures[16];
    memset(textures, 0, sizeof(textures));

    int running = 1;
    while (running) {
        pthread_mutex_lock(&statsMutex);
        if (!keepRunning) {
            pthread_mutex_unlock(&statsMutex);
            break;
        }
        pthread_mutex_unlock(&statsMutex);

        pthread_mutex_lock(&bufferMutex);
        while (bufferHead == bufferTail) {
            pthread_cond_wait(&bufferNotEmpty, &bufferMutex);
        }

        PDU frame = buffer[bufferHead];
        bufferHead = (bufferHead + 1) % bufferSize;
        pthread_cond_signal(&bufferNotFull);
        pthread_mutex_unlock(&bufferMutex);
        displayTimeFromPDU(&frame, renderer, textures);
        usleep(frameInterval);
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

void readReceptionPort(const char* filename, int* port) {
    FILE* file = fopen(filename, "r");
    fscanf(file, "%d", port);
    fclose(file);
}

void* listenForExit(void* arg) {
    char input[10];
    while (1) {
        printf("Press 'Q' to stop execution and see statistics: \n");
        scanf("%s", input);
        if (strcmp(input, "Q") == 0) {
            pthread_mutex_lock(&statsMutex);
            keepRunning = 0;
            pthread_mutex_unlock(&statsMutex);

            pthread_cond_broadcast(&bufferNotEmpty);
            pthread_cond_broadcast(&bufferNotFull);
            break;
        }
    }
    return NULL;
}

void printStatistics() {
    int expectedFrames = lastFrameCount - firstFrameCount + 1;
    int packetLoss = expectedFrames - framesCounter;
    int packetLossBuffer = expectedFrames - framesCounter + skippedFrames;
    printf("\n--- Statistics ---\n");
    printf("Total times buffer reseted: %d\n", bufferReset);
    printf("Total frames lost without buffer reset: %d\n", packetLoss);
    printf("Total frames lost with buffer reset: %d\n", packetLossBuffer);
}

int main() {
    pthread_t receiverThread, displayThread, exitThread;

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    readReceptionPort("client.txt", &receptionPort);

    buffer = malloc(bufferSize * sizeof(PDU));

    pthread_create(&receiverThread, NULL, receivePDU, NULL);
    pthread_create(&displayThread, NULL, displayClock, NULL);
    pthread_create(&exitThread, NULL, listenForExit, NULL);

    pthread_join(exitThread, NULL);
    pthread_join(receiverThread, NULL);
    pthread_join(displayThread, NULL);

    printStatistics();

    IMG_Quit();
    SDL_Quit();

    return 0;
}
