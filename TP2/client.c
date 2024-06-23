/*
    Serviços de Rede & Aplicações Multimédia, TP-2
    Ano Letivo 2023/2024
    Gustavo Oliveira - A83582
    Jose Peleja - A84436
    Marco Araujo - A89387
*/

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

#define DIGIT_IMAGE_SIZE 2145  // Size of each digit image

// Structure representing a PDU (Protocol Data Unit)
typedef struct {
    int A;
    int F;
    int Framecount;
    char digitImages[16][DIGIT_IMAGE_SIZE];
} PDU;

// Global variables for the circular buffer and thread control
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

/*
    Function to update the buffer and frame interval based on the A and F values from the PDU.
*/
void updateBufferAndInterval(int A, int F) {
    pthread_mutex_lock(&bufferMutex);

    // Calculate the new buffer size and new frame interval
    int newBufferSize = pow(10, F) * A;
    int newFrameInterval = 1000000 / pow(10, F);

    // Update the buffer if the size is different
    if (newBufferSize != bufferSize) {
         // Allocate memory for a new buffer with the desired new size
        PDU *newBuffer = malloc(newBufferSize * sizeof(PDU));
        if (bufferHead <= bufferTail) {
             // Copy the buffer segment from bufferHead to bufferTail
            memcpy(newBuffer, buffer + bufferHead, (bufferTail - bufferHead) * sizeof(PDU));
        } else {
            int firstPartSize = bufferSize - bufferHead;
            // Copy the first part, which goes from bufferHead to the end of the buffer
            memcpy(newBuffer, buffer + bufferHead, firstPartSize * sizeof(PDU));
            // Copy the second part, which goes from the start of the buffer to bufferTail
            memcpy(newBuffer + firstPartSize, buffer, bufferTail * sizeof(PDU));
        }

        bufferSize = newBufferSize;
        bufferTail = (bufferTail - bufferHead + bufferSize) % bufferSize;
        bufferHead = 0;

        free(buffer);
        buffer = newBuffer;
    }

    frameInterval = newFrameInterval;

    printf("Buffer Size: %d \n", bufferSize);
    printf("Frame Interval %d \n", frameInterval);

    pthread_mutex_unlock(&bufferMutex);
}

/*
    Function running in a separate thread to receive PDUs via UDP.
*/
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
        perror("Error binding socket");
        close(udpSocket);
        pthread_exit(NULL);
    }

while (1) {
    // Lock the statsMutex to safely check the keepRunning flag
    pthread_mutex_lock(&statsMutex);
    if (!keepRunning) {
        // If keepRunning is false, unlock the mutex and break the loop to stop the thread
        pthread_mutex_unlock(&statsMutex);
        break;
    }
    // Unlock the mutex since the check is complete and keepRunning is true
    pthread_mutex_unlock(&statsMutex);

    recvfrom(udpSocket, &pdu, sizeof(PDU), 0, NULL, NULL);
    framesCounter++;

    if (firstFrameCount == 0) {
        firstFrameCount = pdu.Framecount;
    }
    // Update the last frame count with the current frame's count
    lastFrameCount = pdu.Framecount;

    // Check if the buffer size or frame interval needs to be updated
    if (pdu.A != bufferSize / pow(10, pdu.F) || 1000000 / pow(10, pdu.F) != frameInterval) {
        updateBufferAndInterval(pdu.A, pdu.F);
    }

    // Lock the bufferMutex to safely modify the circular buffer
    pthread_mutex_lock(&bufferMutex);
    // Check if the buffer is full
    if ((bufferTail + 1) % bufferSize == bufferHead) {
        // Calculate the number of frames to be skipped due to buffer overflow
        int skipped = (bufferTail + bufferSize - bufferHead) % bufferSize;
        printf("Buffer full. Resynchronizing by discarding %d frames.\n", skipped);
        skippedFrames += skipped;
        // Reset the buffer head and tail pointers
        bufferHead = 0;
        bufferTail = 0;
        // Increment the buffer reset counter
        bufferReset++;
    }

    buffer[bufferTail] = pdu;
    // Move the bufferTail pointer to the next position, wrapping around if necessary
    bufferTail = (bufferTail + 1) % bufferSize;
    // Signal that the buffer is not empty (for any threads waiting to consume data)
    pthread_cond_signal(&bufferNotEmpty);
    // Unlock the buffer mutex since the buffer modification is complete
    pthread_mutex_unlock(&bufferMutex);
    }

    close(udpSocket);
    return NULL;
}

/*
    Function to load a texture from memory using SDL.
    @param renderer - the SDL renderer
    @param data - the image data in the PDU
    @param size - the size of the image data
    @return the SDL texture created or NULL in case of an error
*/
SDL_Texture* loadTextureFromPDU(SDL_Renderer *renderer, const char *data, int size) {
    if (size == 0 || data[0] == '\0') {
        return NULL;
    }
    SDL_RWops *rw = SDL_RWFromConstMem(data, size);  // Create an SDL_RWops structure from the image data

    SDL_Surface *surface = IMG_Load_RW(rw, 1);  // Load an SDL_Surface from the RWops structure
    if (!surface) {
        printf("Error loading image from PDU: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);  // Create a texture from the surface
    SDL_FreeSurface(surface);  // Free the surface as it is no longer needed
    return texture;
}

/*
    Function to display the time from the PDU using SDL.
    @param pdu - the received PDU
    @param renderer - the SDL renderer
    @param textures - array of SDL textures
*/
void displayTimeFromPDU(PDU *pdu, SDL_Renderer *renderer, SDL_Texture **textures) {
    SDL_Rect dst;
    dst.y = 80;  // Set the y position of the destination rectangle
    dst.w = 40;  // Set the width of the destination rectangle
    dst.h = 40;  // Set the height of the destination rectangle
    for (int i = 0; i < 16; i++) {
        if (textures[i] != NULL) {
            SDL_DestroyTexture(textures[i]);  // Destroy the old texture if it exists
        }
        textures[i] = loadTextureFromPDU(renderer, pdu->digitImages[i], DIGIT_IMAGE_SIZE);  // Load the new texture from pdu
        if (textures[i] == NULL) {
            continue;  // Skip if the texture couldn't be loaded or its empty
        }
        dst.x = 0 + i * 64;  // Set the x position of the destination rectangle
        SDL_RenderCopy(renderer, textures[i], NULL, &dst);  // Copy the texture to the renderer at the specified position
    }
    SDL_RenderPresent(renderer);  // Present the renderer, updating the screen with the new content
}

/*
    Function running in a separate thread to display the clock using SDL.
*/
void* displayClock(void *arg) {
    SDL_Window *window = SDL_CreateWindow("Digital Clock", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1000, 200, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Error creating window: %s\n", SDL_GetError());
        pthread_exit(NULL);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);  // Create an accelerated renderer
    SDL_Texture *textures[16];  // Array to store textures for the digit images
    memset(textures, 0, sizeof(textures));  // Initialize the texture array to NULL

    int running = 1;  // Flag to control the main loop
    while (running) {
        // Lock the stats mutex to check the running status
        pthread_mutex_lock(&statsMutex);
        if (!keepRunning) {
            pthread_mutex_unlock(&statsMutex);
            break;
        }
        pthread_mutex_unlock(&statsMutex);

        // Lock the buffer mutex to check if there is data to display
        pthread_mutex_lock(&bufferMutex);
        while (bufferHead == bufferTail) {
            pthread_cond_wait(&bufferNotEmpty, &bufferMutex);  // Wait until the buffer is not empty
        }

        // Get the next frame from the buffer
        PDU frame = buffer[bufferHead];
        bufferHead = (bufferHead + 1) % bufferSize;
        pthread_cond_signal(&bufferNotFull);  // Signal that the buffer is not full
        pthread_mutex_unlock(&bufferMutex);

        displayTimeFromPDU(&frame, renderer, textures);  // Display the time from the PDU
        usleep(frameInterval);  // Sleep for the frame interval

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;  // Exit the loop if the window is closed
            }
        }
    }

    // Clean up textures and renderer
    for (int i = 0; i < 16; i++) {
        if (textures[i]) {
            SDL_DestroyTexture(textures[i]);
        }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    pthread_exit(NULL);
}

/*
    Function to read the reception port from a configuration file.
*/
void readReceptionPort(const char* filename, int* port) {
    FILE* file = fopen(filename, "r");
    fscanf(file, "%d", port);
    fclose(file);
}

/*
    Function running in a separate thread to listen for user input to exit the program.
*/
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

/*
    Function to print the transmission statistics.
*/
void printStatistics() {
    int expectedFrames = lastFrameCount - firstFrameCount + 1;
    int packetLoss = expectedFrames - framesCounter;
    int packetLossBuffer = expectedFrames - framesCounter + skippedFrames;
    printf("\n--- Statistics ---\n");
    printf("Total times buffer reset: %d\n", bufferReset);
    printf("Total frames lost without buffer reset: %d\n", packetLoss);
    printf("Total frames lost with buffer reset: %d\n", packetLossBuffer);
}

/*
    Main function of the program.
*/
int main() {
    pthread_t receiverThread, displayThread, exitThread;

    SDL_Init(SDL_INIT_VIDEO);  // Initialize the SDL video subsystem
    IMG_Init(IMG_INIT_PNG);    // Initialize the SDL image subsystem 

    readReceptionPort("client.txt", &receptionPort); // Read the reception port from the configuration file

    buffer = malloc(bufferSize * sizeof(PDU));

    pthread_create(&receiverThread, NULL, receivePDU, NULL); // Create the thread to receive PDUs
    pthread_create(&displayThread, NULL, displayClock, NULL); // Create the thread to display the clock
    pthread_create(&exitThread, NULL, listenForExit, NULL); // Create the thread to listen for user input to exit

    pthread_join(exitThread, NULL);
    pthread_join(receiverThread, NULL);
    pthread_join(displayThread, NULL);

    printStatistics();

    IMG_Quit();  // Quit the SDL image subsystem
    SDL_Quit();  // Quit the SDL video subsystem

    return 0;
}
