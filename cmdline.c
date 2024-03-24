#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
 

int main(int argc, char *argv[]){
    char *sizeofbuffer;

    sizeofbuffer = malloc(strlen(argv[1])+1);

    strcpy(sizeofbuffer, argv[1]);

    printf("%s", sizeofbuffer);
     if( strcmp(sizeofbuffer, "-86")  == 0){
        printf("86");
    } 
    else{   
        if( strcmp(sizeofbuffer, "-32")  == 0){
            printf("32");
        } 
        else{
            printf("default");
        }
    }
}