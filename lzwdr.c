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
int indexOfPattern = 1;

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
    @param dictionary - dictionary being used.
    @param pattern - new pattern that we want to insert in the function.
    @param lenght - length of the pattern.
    @return - this function returns an integer with the value: 0 if the dictionary is full OR the value of index associated with the inserted pattern.
*/
int insert(trieNode **dictionary, unsigned char *pattern){
    if(*dictionary == NULL){
        *dictionary = createNewNode();
    }
    unsigned char *text = pattern;
    trieNode *temporaryNode = *dictionary;

    for(int level = 0; level < sizeof(pattern); level++){
        if(temporaryNode->children[text[level]] == NULL){
            temporaryNode->children[text[level]] = createNewNode();
        }
        temporaryNode = temporaryNode->children[text[level]];
    }

    if(indexOfPattern >= sizeOfTheDictionary){
        return 0;
    } else {
        temporaryNode->indexOfPattern = indexOfPattern;
        ++indexOfPattern;
        return  temporaryNode->indexOfPattern;
    }
}

/*
    Function to search for a node in the trie.
    @param dictionary - dictionary being used.
    @param pattern - new pattern that we want to insert in the function.
    @param lenght - length of the pattern.
    @return - this function returns an integer with the value: 0 if the pattern doesn't exists OR the value of index associated with the inserted pattern.
*/
int search(trieNode *dictionary, unsigned char *pattern){
    unsigned char *text = pattern;
    trieNode * temporaryNode = dictionary;
    printf("alo pqp");

    for(int i = 0; i < sizeof(pattern) ; i++){
        if(temporaryNode->children[text[i]] == NULL){
            return 0;
        }

        unsigned char newText = text[i];
        temporaryNode = temporaryNode->children[newText];
    }

    return temporaryNode->indexOfPattern;
}

/*
    Function to add a pattern code to the output string.
    @param code - the code to be inserted in the output.
    @param string - current output string.
    @return - output string with the added code.
*/
char* output(int code, char* string){
    string = realloc(string, sizeof(string) + 1);
    strcat(string, sprintf(string, "%d", code));

    return string;
}

/*
    Function to concatenate patterns.
    @param patternA - pattern A.
    @param patternB - pattern B.
    @return - this function returns the concatenation of the two input patterns.
*/
unsigned char* concat(unsigned char* patternA, unsigned char* patternB){
    unsigned char* concatenation = malloc((sizeof(patternA) + sizeof(patternB)) * sizeof(unsigned char));
    
    for(int i = 0; i < sizeof(patternA); i++){
        concatenation[i] = patternA[i];
    }
    for (int i = 0; i < sizeof(patternB); i++) {
        concatenation[i + sizeof(patternA)] = patternB[i];
    }
        
    return concatenation;
}

/*
    Function to invert an array of unsigned char.
    @param originalArray - the array to be inverted.
    @return - this function returns an unsigned char array with the inverted value of the originalArray.
*/
unsigned char* reverse(unsigned char *originalArray) {
    unsigned char* invertedArray = malloc(sizeof(originalArray) * sizeof(unsigned char));
    
    for (int i = 0; i < sizeof(originalArray); i++) {
        invertedArray[i] = originalArray[sizeof(originalArray) - 1 - i];
    }
    
    return invertedArray;
}

/*
    Function to process a block of a file and compress it using lzwdr algorithm.
    @param block - block to be processed.
    @param blockSize - size of the block to be processed.
    @param dictionary - dictionary being used.
    @return - this function returns the output string after the block has been processed by the lzwdr algorithm.
*/
char* lzwdr(unsigned char *block, size_t blockSize, trieNode *dictionary) {
    char* outputString;
    unsigned char* patternA = malloc(1 * sizeof(unsigned char));
    unsigned char* patternB  = malloc(1 * sizeof(unsigned char));
    int code = 0;
    int index = 0;
    int i = 0;
    patternA = block[index];
    index++; 
    
    while(index + i < blockSize) {
        //Processing block to find the bigger patternB after patternA already in D.
        printf("alo");
        code = search(patternA, dictionary);
        
        patternB = block[index];
        while(search(concat(patternB, block[index+i]), dictionary)) {
            unsigned char* aux = concat(patternB, block[index+i]);
            patternB = realloc(patternB, sizeof(aux)+1);
            patternB = aux;
            i = i + 1;
        }

        //Send the code of patternA to the output. 
        output(code, outputString);
        
        //Insert in the dictionary all the new patterns while the dictionary is not full
        int j = 0;
        int t = sizeOfTheDictionary - 1;
        
        while(j <= i && t < sizeOfTheDictionary) {
            t = insert(concat(patternA, patternB[j]), dictionary);
            
            if(t < sizeOfTheDictionary){
                 t = insert(reverse(concat(patternA, patternB[j])), dictionary);
            }
        
            j++;
        }
        
        if(index + i > blockSize){
            output(search(patternB, dictionary), outputString);
        } else{
            index = index + i;
            patternA =  realloc(patternA,sizeof(patternB)+1);
            patternA = patternB;
        }
    }
        
    free(patternA);
    free(patternB);
    return outputString;
}

/*
    argv[1] - name of the file that we need to compress.
    argv[2] - string value associated with the size of the block being utilized.
    argv[3] - string value associated with the maximum size of the dictionary.
TODO   argv[4] - string value associated with the type of dictionary.
TODO   argv[5] - string value associated with the dictionary management options.
*/
int main(int argc, char *argv[]){
    trieNode * dictionary = NULL;
    FILE *fileToCompress;
    FILE *outputFile;
    unsigned char *buffer;
    size_t blockSize;
    long fileSize;
    size_t bytesRead;
    clock_t start, end;
    unsigned char *patternA;
    unsigned char *patternB;
    char* fileName;
    char* blockComparator;
    char* sizeAux;
    //populate the dictionary
    for (int i  = 0; i < ALPHABET_SIZE; i++) {
        unsigned char ind[1];
        ind[0] = (unsigned char)i;
        insert(&dictionary, ind);
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
    buffer = malloc(blockSize * sizeof(unsigned char));
    free(blockComparator);

    start = clock();
    while (fileSize > 0) {
        printf("%d \n",blockSize);
        bytesRead = fread(buffer, 1, blockSize, fileToCompress); 
        printf("%d \n",sizeof(buffer));
        printf("%d \n",bytesRead);
        lzwdr(buffer, bytesRead, dictionary);
            
        fileSize -= bytesRead;
        free(bytesRead);
    }
    end = clock();
    double duration = ((double)end - start)/CLOCKS_PER_SEC; //duration of the compression in seconds.

    fclose(fileToCompress);
    free(buffer);

    return 0;
}
