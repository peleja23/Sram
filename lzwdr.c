/*
    Serviços de Rede & Aplicações Multimédia, TP-1
    Ano Letivo 2022/2023
    Gustavo Oliveira - A83582
    Jose Peleja - A84436
    Marco Araujo - A89387
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define ALPHABET_SIZE 256

//Declaration of global variables
long sizeOfTheDictionary = 0;
int inxdexOfPattern = 1;
bool dictionaryIsFull = false;

typedef struct trieNode {
    struct trieNode *children[ALPHABET_SIZE]; 
    int indexOfPattern;
    int numberOfSearches;
} trieNode;


/*
    Function to creat a new node in the trie.
    @return newNode, returns the new node.
*/
trieNode *createNewNode(){
    trieNode *newNode = malloc(sizeof *newNode);
    
    if(newNode){
        newNode->indexOfPattern = 0;
        
        for(int i = 0; i < ALPHABET_SIZE; i++){
            newNode->children[i] = NULL;
        }
    }
    
    return newNode;
}

/*
    Function to add a new node in the trie.
    @param dictionary - dictionary being used
    @param pattern - new pattern that we want to insert in the function
    @param lenght - length of the pattern
    @return - this function returns an integer with the value: 0 if the dictionary is full OR the value of index associated with the inserted pattern.
*/
int insertNode(trieNode **dictionary, unsigned char *pattern, int length){

    if(*dictionary == NULL){
        *dictionary = createNewNode();
    }
    unsigned char *text = pattern;
    trieNode *temporaryNode = *dictionary;

    for(int level = 0; level < length; level++){
        if(temporaryNode->children[text[level]] == NULL){
            temporaryNode->children[text[level]] = createNewNode();
        }
        temporaryNode = temporaryNode->children[text[level]];
    }

    if(dictionaryIsFull){
        return 0;
    } else {
        temporaryNode->indexOfPattern = inxdexOfPattern;
        ++inxdexOfPattern;
        return  temporaryNode->indexOfPattern;
    }
}


/*
    Function to search for a node in the trie.
    @param dictionary - dictionary being used
    @param pattern - new pattern that we want to insert in the function
    @param lenght - length of the pattern
    @return - this function returns an integer with the value: 0 if the pattern doesn't exists OR the value of index associated with the inserted pattern.
*/
int searchInTrie(trieNode *dictionary, unsigned char *pattern, int length){
    printf("ola");
    unsigned char *text = pattern;
    trieNode * temporaryNode = dictionary;

    for(int i = 0; i < length ; i++){
        if(temporaryNode->children[text[i]] == NULL){
            return 0;
        }

        unsigned char newText = text[i];
        temporaryNode = temporaryNode->children[newText];
    }

    return temporaryNode->indexOfPattern;
}

unsigned char* invertArray(unsigned ogArr [], int size) {
    unsigned char* invertedArr = (unsigned char*)malloc(size * sizeof(ogArr));
    int end = size - 1;
        for (int i = 0; i < size; i++) {
            invertedArr[i] = arr[end - i];
        }
    return invertedArr;
}

/*
    argv[1] - name of the file that we need to compress.
    argv[2] - string value associated with the size of the block being utilized.
TODO   argv[3] - string value associated with the maximum size of the dictionary.
TODO   argv[4] - string value associated with the type of dictionary.
TODO   argv[5] - string value associated with the dictionary management options.
*/
int main(int argc, char *argv[]){
    
    trieNode * dictionary = NULL;
    FILE *fileToCompress;
    FILE *outputFile;
    unsigned char *buffer;
    int blockSize;
    long fileSize;
    size_t blockCompressed;
    clock_t start, end;
    unsigned char *prefixA;
    unsigned char *prefixB;
    char* fileName;
    char* blockComparator;
    char* sizeAux;
    //populate the dictionary
    for (int i  = 0; i < ALPHABET_SIZE; i++) {
        unsigned char ind[1];
        ind[0] = (unsigned char)i;
        insertNode(&dictionary, ind, 1);
    }
    //Memory allocation + string copy for values received from the arguments
    fileName = malloc(strlen(argv[1])+1);
    strcpy(fileName, argv[1]);
    blockComparator = malloc(strlen(argv[2])+1);
    strcpy(blockComparator, argv[2]);
    sizeAux = malloc(strlen(argv[3])+1);
    strcpy(sizeAux, argv[3]);

    fileToCompress = fopen(fileName, "rb");
    if (fileToCompress == NULL) {
        fputs("Erro ao abrir arquivo", stderr);
        exit(1);
    }
    free(fileName);

    fseek(fileToCompress, 0, SEEK_END);
    fileSize = ftell(fileToCompress);
    rewind(fileToCompress); 
    //Determining the size of the blocks being read from the file dependent on the value of argv[2].
    if( strcmp(blockComparator, "-86") == 0){
        blockSize = 88064;
    } 
    else{
        
        if( strcmp(blockComparator, "-32") == 0){
            blockSize = 32768;
        } 
        else{
            blockSize = 65536;
        }
    }
    if( strcmp(sizeAux, "-12") == 0){
        sizeOfTheDictionary = 4096;
    } 
    else{
        if( strcmp(sizeAux, "-24") == 0){
            sizeOfTheDictionary = 16777216;
        } 
        else{
            sizeOfTheDictionary = 65536;
        }
    }
    buffer = malloc((unsigned char) malloc(sizeof(blockSize)));
    free(blockComparator);

    start = clock();
    while (fileSize > 0) {
        blockCompressed = fread(buffer, 1, blockSize, fileToCompress); 
        //TODO
        //prefixA = malloc
        //prefixB = malloc
        for(int i=0; i<blockCompressed; i++){
            unsigned char aux;
            prefixA = buffer[i];
            printf("%c", prefixA);
            prefixB = buffer[i + 1];
            printf("%c", prefixB);
            searchInTrie(dictionary, prefixA, 1);
            if(searchInTrie(dictionary, prefixA, 1) == -1){
                insertNode(&dictionary, *prefixA, 1);
            }
            if(searchInTrie(dictionary, prefixB, 1) == -1){
                insertNode(&dictionary, *prefixB, 1);
            }

        }

        fileSize -= blockCompressed;
        free(blockCompressed);
    }
    end = clock();
    double duration = ((double)end - start)/CLOCKS_PER_SEC; //duration of the compression in seconds.

    fclose(fileToCompress);
    free(buffer);

    return 0;
}