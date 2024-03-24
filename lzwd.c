/*
    Serviços de Rede & Aplicações Multimédia, TP-1
    Ano Letivo 2022/2023
    Gustavo Oliveira - A83582
    Rodrigo Bobzien - A84215
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define ALPHABET_SIZE 256

//Declaration of global variables
int sizeOfTheDictionary = 0;
int inxdexOfPattern = 1;
bool dictionaryIsFull = false;

typedef struct trieNode {
    struct trieNode *children[ALPHABET_SIZE]; 
    int indexOfPattern;
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
    @return - this function returns an integer with the value: 0 if the dictionary is full OR the value of index associated with the inserted pattern.
*/
int insertNode(trieNode **dictionary, char *pattern){

    if(*dictionary == NULL){
        *dictionary = createNewNode();
    }

    unsigned char *text = (unsigned char *)pattern;
    trieNode *temporaryNode = *dictionary;
    int lenghtText =  strlen(pattern);

    for(int level = 0; level < lenghtText; level++){
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
    @return - this function returns an integer with the value: 0 if the pattern doesn't exists OR the value of index associated with the inserted pattern.
*/
int searchInTrie(trieNode *dictionary, char *pattern){
    unsigned char *text = (unsigned char *)pattern;
    int lenght = strlen(pattern);

    trieNode * temporaryNode = dictionary;

    for(int i = 0; i < lenght ; i++){
        if(temporaryNode->children[text[i]] == NULL){
            return 0;
        }

        unsigned char newText = text[i];
        temporaryNode = temporaryNode->children[newText];
    }

    return temporaryNode->indexOfPattern;
}

/*
    argv[1] - name of the file that we need to compress.
    argv[2] - string value associated with the size of the block being utilized.
TODO   argv[3] - string value associated with the maximum size of the dictionary.
TODO   argv[4] - string value associated with the type of dictionary.
TODO   argv[5] - string value associated with the dictionary management options.
*/
int main(int argc, char *argv[]){

    trieNode * dictionry = NULL;
    FILE *fileToCompress;
    FILE *outputFile;
    char *buffer;
    int blockSize;
    long fileSize;
    size_t blockCompressed;
    clock_t start, end;
    char *prefixA;
    char *prefixB;
    char* fileName;
    char* blockComparator;

    //Memory allocation + string copy for values received from the arguments
    fileName = malloc(strlen(argv[1])+1);
    strcpy(fileName, argv[1]);
    blockComparator = malloc(strlen(argv[2])+1);
    strcpy(blockComparator, argv[2]);

    fileToCompress = fopen(fileName, "r");
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

    buffer = malloc((char) malloc(sizeof(blockSize)));
    free(blockComparator);
    
    // TODO 
    
    start = clock();
    while (fileSize > 0) {
        blockCompressed = fread(buffer, 1, blockSize, fileToCompress);
        if (blockCompressed != blockSize && !feof(fileToCompress)) {
            fputs("Erro ao ler arquivo", stderr);
            exit(1);    
        }

        //TODO 

        fileSize -= blockCompressed;
    }
    end = clock();
    double duration = ((double)end - start)/CLOCKS_PER_SEC; //duration of the compression in seconds.

    fclose(fileToCompress);
    free(buffer);

    return 0;
}