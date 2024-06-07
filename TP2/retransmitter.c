#include <winsock2.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define PORT 6666
#define MAX_BUFFER 65536
#define DIGIT_IMAGE_SIZE 4096  

int main(int argc, char* argv[]){
    int M = 1;
    int N = 1;
    int P = 1;
    if (argc > 1) {
        M = atoi(argv[1]);
    }
    if (argc > 2) {
        N = atoi(argv[2]);
    }
    if (argc > 3) {
        P = atoi(argv[3]);
    }

}