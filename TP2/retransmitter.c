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
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>

#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 2145
#define BUFFER_SIZE 30000 

// Definition of a structure PDU 
typedef struct {
    int A;                      
    int F;                
    int Framecount;             // Frame count
    char digitImages[16][DIGIT_IMAGE_SIZE];  // Images
} PDU;

// Circular buffer to store PDUs
PDU buffer[BUFFER_SIZE];
int bufferHead = 0;  // Head of the buffer
int bufferTail = 0;  // Tail of the buffer

// Global variables
int N;  
int P;  
int M;  
int serverPort;
int clientPort;
char serverIp[INET_ADDRSTRLEN];
int keepRunning = 1;  // Flag to keep the program running
long skippedFrames = 0;  // Counter of skipped frames
double totalPausedTime = 0.0;  // Total pause time

// Mutexes and condition variables for thread synchronization
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t statsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bufferNotEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t bufferNotFull = PTHREAD_COND_INITIALIZER;

/*
    Function to read the retransmitter configuration from a file.
    @param filename - path to the configuration file
    @param serverPort - pointer to store the server port
    @param clientPort - pointer to store the client port
    @param serverIp - buffer to store the server IP address
*/
void readRetransmitterConfig(const char* filename, int* serverPort, int* clientPort, char* serverIp) {
    FILE* file = fopen(filename, "r");
    fscanf(file, "%d", serverPort);
    fscanf(file, "%d", clientPort);
    fscanf(file, "%s", serverIp);
    fclose(file);
}

/*
    Thread function to receive PDUs from the server.
    This function runs in a separate thread and is responsible for receiving PDUs sent by the server
    and storing them in the circular buffer.
*/
void* receiveFromServer(void* arg) {
    int sock;
    struct sockaddr_in server, client;
    PDU pdu;
    socklen_t clientLen = sizeof(client);
    time_t lastSkipTime = time(NULL);

    // Create the UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(serverPort);

    // Bind the socket to the server port
    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Check if the program should keep running and Locking the mutex to check the running status
        pthread_mutex_lock(&statsMutex);
        if (!keepRunning) {
            // Unlocking the mutex once the running status is verified
            pthread_mutex_unlock(&statsMutex);
            break;
        }
        // Unlocking the mutex once the running status is verified
        pthread_mutex_unlock(&statsMutex);

        // Receive PDU from the server
        recvfrom(sock, &pdu, sizeof(PDU), 0, (struct sockaddr *)&client, &clientLen);

        // Check if the reception of PDU should be skipped based on the M interval
        time_t currentTime = time(NULL);
        if (difftime(currentTime, lastSkipTime) >= M) {
            printf("Skipping PDU due to M seconds interval...\n");
            lastSkipTime = currentTime;
            pthread_mutex_lock(&statsMutex);
            skippedFrames++;
            pthread_mutex_unlock(&statsMutex);
            continue;
        }

        pthread_mutex_lock(&bufferMutex);
        while ((bufferTail + 1) % BUFFER_SIZE == bufferHead) {
            // Wait until the buffer is not full
            pthread_cond_wait(&bufferNotFull, &bufferMutex);
        }
        // Insert the received PDU into the circular buffer
        buffer[bufferTail] = pdu;
        bufferTail = (bufferTail + 1) % BUFFER_SIZE;
        // Signal that the buffer is not empty
        pthread_cond_signal(&bufferNotEmpty);
        pthread_mutex_unlock(&bufferMutex);
    }

    close(sock);
    return NULL;
}

/*
    Thread function to retransmit PDUs to the client.
    This function runs in a separate thread and is responsible for removing PDUs from the circular buffer
    and retransmitting them to the client.
*/
void* retransmitToClient(void* arg) {
    int sock;
    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);
    int frameCount = 0;

    // Create the UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = inet_addr(serverIp);
    client.sin_port = htons(clientPort);

    while (1) {
        // Check if the program should keep running and Locking the mutex to check the running status
        pthread_mutex_lock(&statsMutex);
        if (!keepRunning) {
            // Unlocking the mutex once the running status is verified
            pthread_mutex_unlock(&statsMutex);
            break;
        }
        // Unlocking the mutex once the running status is verified
        pthread_mutex_unlock(&statsMutex);

        pthread_mutex_lock(&bufferMutex);
        while (bufferHead == bufferTail) {
            // Wait until the buffer is not empty
            pthread_cond_wait(&bufferNotEmpty, &bufferMutex);
        }
         // Remove the PDU from the buffer
        PDU pdu = buffer[bufferHead];
        bufferHead = (bufferHead + 1) % BUFFER_SIZE;
        pthread_cond_signal(&bufferNotFull);
        pthread_mutex_unlock(&bufferMutex);

        frameCount++;
        printf("Retransmitting Frame Number: %d -> A: %d, F: %d, Framecount: %d\n", frameCount, pdu.A, pdu.F, pdu.Framecount);

        // Pause after N frames
        if (frameCount % N == 0) {
            printf("Pausing for %d seconds...\n", P);
            sleep(P);
            pthread_mutex_lock(&statsMutex); // Lock the mutex to protect access to the totalPausedTime variable
            // Calculate the duration of the pause and add it to the total paused time
            totalPausedTime += P;
            pthread_mutex_unlock(&statsMutex); // Unlock the mutex
        }
        usleep(1);
        sendto(sock, &pdu, sizeof(PDU), 0, (struct sockaddr *)&client, clientLen);
    }

    close(sock);
    return NULL;
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
            pthread_mutex_lock(&statsMutex);
            keepRunning = 0; // Set the flag to 0 to stop the main loops
            pthread_mutex_unlock(&statsMutex);

            // Signal all threads waiting on the conditions
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
    pthread_mutex_lock(&statsMutex);
    printf("\n--- Statistics ---\n");
    printf("Total skipped frames: %ld\n", skippedFrames);
    printf("Total paused time: %.0f seconds\n", totalPausedTime);
    pthread_mutex_unlock(&statsMutex);
}

/*
    Main function.
    argv[1] - value for N (number of frames before a pause)
    argv[2] - value for P (pause duration in seconds)
    argv[3] - value for M (interval to skip a frame in seconds)
*/
int main(int argc, char* argv[]) {
    N = 100;  
    P = 2;   
    M = 9; 
    if (argc > 1) {
        N = atoi(argv[1]);
    }
    if (argc > 2) {
        P = atoi(argv[2]);
    }
    if (argc > 3) {
        M = atoi(argv[3]);
    }

    readRetransmitterConfig("retransmitter.txt", &serverPort, &clientPort, serverIp);

    printf("Starting retransmitter with N=%d, P=%d, M=%d\n", N, P, M);

    pthread_t receiverThread, retransmitterThread, exitThread;
    pthread_create(&receiverThread, NULL, receiveFromServer, NULL);
    pthread_create(&retransmitterThread, NULL, retransmitToClient, NULL);
    pthread_create(&exitThread, NULL, listenForExit, NULL);

    pthread_join(exitThread, NULL);
    pthread_join(receiverThread, NULL);
    pthread_join(retransmitterThread, NULL);

    printStatistics();

    return 0;
}
