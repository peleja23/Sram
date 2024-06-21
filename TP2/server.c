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
#include <math.h>

#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145  

// Definition of a structure PDU
typedef struct {
    int A;     
    int F;  
    int Framecount;  // Frame count
    char digitImages[16][DIGIT_IMAGE_SIZE];  // Images
} PDU;

// Array of file paths for digit images
char* digitFiles[] = {
    "digitos/zero.png", "digitos/um.png", "digitos/dois.png",
    "digitos/tres.png", "digitos/quatro.png", "digitos/cinco.png",
    "digitos/seis.png", "digitos/sete.png", "digitos/oito.png",
    "digitos/nove.png"
};
const char* separatorFile = "digitos/separador.png"; // File path for separator image

// Buffers to store digit and separator images
char digitImageBuffers[10][DIGIT_IMAGE_SIZE];
char separatorImageBuffer[DIGIT_IMAGE_SIZE];

// Global variables
PDU pdu;
long totalFramesSent = 0;
struct timespec startTime, endTime;
int keepRunning = 1;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/*
    Function to load an image from a file into a buffer.
    @param filename - path to the image file
    @param buffer - buffer to store the image data
    @param bufferSize - size of the buffer
*/
void loadImage(const char* filename, char* buffer, int bufferSize) {
    FILE* file = fopen(filename, "rb");
    fread(buffer, 1, bufferSize, file);
    fclose(file);
}

/*
    Function to preload all digit and separator images into memory.
*/
void preloadImages() {
    for (int i = 0; i < 10; i++) {
        loadImage(digitFiles[i], digitImageBuffers[i], DIGIT_IMAGE_SIZE);
    }
    loadImage(separatorFile, separatorImageBuffer, DIGIT_IMAGE_SIZE);
}

/*
    Function to read server configuration from a file.
    @param filename - path to the configuration file
    @param ip - buffer to store the server IP address
    @param port - pointer to store the server port number
*/
void readServerConfig(const char* filename, char* ip, int* port) {
    FILE* file = fopen(filename, "r");
    fscanf(file, "%d", port);
    fscanf(file, "%s", ip);
    fclose(file);
}

/*
    Function to generate a timestamp string with different precision levels.
    @param F - precision level
    @param buffer - buffer to store the generated timestamp string
*/
void generateTimeString(int F, char* buffer) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm *tm_info = localtime(&ts.tv_sec);
    int microseconds = ts.tv_nsec / 1000;

    switch (F) {
        case 0:
            sprintf(buffer, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            break;
        case 1:
            sprintf(buffer, "%02d:%02d:%02d:%01d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 100000);
            break;
        case 2:
            sprintf(buffer, "%02d:%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 10000);
            break;
        case 3:
            sprintf(buffer, "%02d:%02d:%02d:%03d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 1000);
            break;
        case 4:
            sprintf(buffer, "%02d:%02d:%02d:%04d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 100);
            break;
        case 5:
            sprintf(buffer, "%02d:%02d:%02d:%05d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds / 10);
            break;
        case 6:
            sprintf(buffer, "%02d:%02d:%02d:%06d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, microseconds);
            break;
    }
}

/*
    Function to prepare a PDU with the current timestamp.
    @param pdu - pointer to the PDU to be prepared
    @param F - precision level
    @param A - Maximum recommended delay
    @param Framecount - current frame count
*/
void preparePDU(PDU* pdu, int F, int A, int Framecount) {
    pdu->A = A;
    pdu->F = F;
    pdu->Framecount = Framecount;
    char timestamp[MAX_BUFFER];
    generateTimeString(F, timestamp);
    int imageIndex = 0;
    printf("Frame number : %d ->timestamp:%s\n", Framecount, timestamp);
    for (int i = 0; i < strlen(timestamp); i++) {
        if (timestamp[i] == ':') {
            memcpy(pdu->digitImages[imageIndex], separatorImageBuffer, DIGIT_IMAGE_SIZE);
        } else {
            // Convert char to integer
            int digit = timestamp[i] - '0';
            if (digit >= 0 && digit <= 9) {
                memcpy(pdu->digitImages[imageIndex], digitImageBuffers[digit], DIGIT_IMAGE_SIZE);
            }
        }
        imageIndex++;
    }
}

/*
    Function to send data to the server.
    @param F - precision level
    @param A - Maximum recommended delay
    @param Framecount - current frame count
    @param ip - server IP address
    @param port - server port number
*/
void sendData(int F, int A, int Framecount, const char* ip, int port) {
    int udpSocket;
    struct sockaddr_in server;

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);
    char previousTimestamp[MAX_BUFFER] = {0};
    char currentTimestamp[MAX_BUFFER];

    // Get the start time for transmission
    clock_gettime(CLOCK_REALTIME, &startTime);
    while (1) {
        // Locking the mutex to check the running status
        pthread_mutex_lock(&lock);
        if (!keepRunning) {
            // Unlocking the mutex once the running status is verified
            pthread_mutex_unlock(&lock);
            break;
        }
        // Unlocking the mutex once the running status is verified
        pthread_mutex_unlock(&lock);

        // Generate a new timestamp
        generateTimeString(F, currentTimestamp);
        // If the timestamp has changed, send a new PDU
        if (strcmp(previousTimestamp, currentTimestamp) != 0) {
            Framecount++;
            //Prepares a new PDU to be sent
            preparePDU(&pdu, F, A, Framecount); 
            sendto(udpSocket, &pdu, sizeof(PDU), 0, (struct sockaddr *)&server, sizeof(server));
            strcpy(previousTimestamp, currentTimestamp);
        }
    }

    clock_gettime(CLOCK_REALTIME, &endTime);
    close(udpSocket);
}

/*
    Thread function to listen for user input to exit the program.
*/
void* listenForExit(void* arg) {
    char input[10];
    while (1) {
        printf("Press 'Q' to stop execution and see statistics: \n");
        scanf("%s", input);
        if (strcmp(input, "Q") == 0) {
            // Locking the mutex to update the running status
            pthread_mutex_lock(&lock);
            keepRunning = 0; // Set the flag to 0 to stop the main loop
            pthread_cond_signal(&cond); // Signal the condition variable to unblock any waiting thread
            pthread_mutex_unlock(&lock);
            break;
        }
    }
    return NULL;
}

/*
    Function to print statistics of the PDU transmission.
    @param pdu - pointer to the PDU
*/
void printStatistics(PDU* pdu) {
    double totalTime = (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec) / 1e9;    
    double frameRate = pdu->Framecount / totalTime;

    printf("\n--- Statistics ---\n");
    printf("Total sent Frames: %d\n", pdu->Framecount);
    printf("Execution time: %.3f seconds\n", totalTime); 
    printf("Info Rate: %.0f frames/second\n", frameRate); 
}

/*
    argv[1] - value for precision level
    argv[2] - value for maximum recommended delay
*/

int main(int argc, char* argv[]) {
    preloadImages();  // Preload digit and separator images into memory
    int F = 0;
    int A = 2;
    int Framecount = 0; 
    char ip[INET_ADDRSTRLEN];
    int port;

    readServerConfig("server.txt", ip, &port); // Read server configuration

    if (argc > 1) {
        F = atoi(argv[1]); // Set precision level from command line argument
    }
    if (argc > 2) {
        A = atoi(argv[2]); // Set maximum recommended delay from command line argument
    }

    pthread_t exitThread;
    pthread_create(&exitThread, NULL, listenForExit, NULL); // Create a thread to listen for exit command

    sendData(F, A, Framecount, ip, port); // Start sending data to the server

    pthread_join(exitThread, NULL);
    printStatistics(&pdu);

    return 0;
}