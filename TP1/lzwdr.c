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
int dictionaryCap = 0;
int numberOfCodes = 0;
int compressedSize = 0;

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
    Function to search for a node in the trie.
    @param pattern - new pattern that we want to insert in the function
    @param lenght - length of the pattern
    @param dictionary - dictionary being used
    @return - this function returns an integer with the value: 0 if the pattern doesn't exists OR the value of index associated with the inserted pattern.
*/
int insert(unsigned char *pattern, int length, trieNode **dictionary){
    if(*dictionary == NULL){
        *dictionary = createNewNode();
    }
    
    trieNode *temporaryNode = *dictionary;

    for(int level = 0; level < length; level++){
        if(temporaryNode->children[pattern[level]] == NULL){
            temporaryNode->children[pattern[level]] = createNewNode();
        }
        temporaryNode = temporaryNode->children[pattern[level]];
    }

    if(indexOfPattern >= sizeOfTheDictionary){
        dictionaryCap++;
        return 0;
    } else {
        temporaryNode->indexOfPattern = indexOfPattern;
        if(indexOfPattern > 256 ){
            printf("Padrao adicionado: ");
            for(int level = 0; level < length; level++){
                printf("%c", pattern[level]);
            }
            printf(" - %d\n", temporaryNode->indexOfPattern);
        }else{
            if (indexOfPattern == 256)
            {
                printf("Dicionario inicializado\n");
            }
            
        }

        ++indexOfPattern;
        return  temporaryNode->indexOfPattern;
    }
}

/*
    Function to add a pattern code to the output string.
    @param code - the code to be inserted in the output.
    @param string - current output string.
    @param lenght - length of the string.
    @return - output string with the added code.
*/
int search(unsigned char *pattern, int length, trieNode *dictionary){
    trieNode * temporaryNode = dictionary;
    
    for(int i = 0; i < length; i++){
        if(temporaryNode->children[pattern[i]] == NULL){
            return 0;
        }
        temporaryNode = temporaryNode->children[pattern[i]];
    }
    return temporaryNode->indexOfPattern;
}

/*
    Function to add a pattern code to the output string.
    @param code - the code to be inserted in the output.
    @param string - current output string.
    @param lenght - length of the string.
    @return - output string with the added code.
*/
char* output(int code, char* string, int length){
    char codeString[12];
    sprintf(codeString, "%d,", code);
    strcat(string, codeString);
    numberOfCodes++;
    return string;
}

/*
    Function to concatenate patterns.
    @param patternA - pattern A.
    @param lenghtA - length of pattern A.
    @param patternB - pattern B.
    @param lenghtB - length of pattern B.
    @return - this function returns the concatenation of the two input patterns.
*/
unsigned char* concat(unsigned char* patternA, int lengthA, unsigned char* patternB, int lengthB){
    unsigned char* concatenation = malloc((lengthA + lengthB) * sizeof(unsigned char));
    memcpy(concatenation, patternA, lengthA);
    memcpy(concatenation + lengthA, patternB, lengthB);
    return concatenation;
}

/*
    Function to invert an array of unsigned char.
    @param originalArray - the array to be inverted.
    @param lenght - length of the array to be inverted.
    @return - this function returns an unsigned char array with the inverted value of the originalArray.
*/
unsigned char* reverse(unsigned char *originalArray, int length) {
    unsigned char* invertedArray = malloc(length * sizeof(unsigned char));
    for (int i = 0; i < length; i++) {
        invertedArray[i] = originalArray[length - 1 - i];
    }
    return invertedArray;
}

int numberOfBits(unsigned int num) {
    int bits = 0;
    if (num <= 256){
        bits = 8;
    }
    if (num > 256 && num <= 512){
        bits = 9;
    }
    if(num > 512 && num <= 1024){
        bits = 10;
    }
        if(num > 1024 && num <= 2048){
        bits = 11;
    }
        if(num > 2048 && num <= 4096){
        bits = 12;
    }
        if(num > 4096 && num <= 8192){
        bits = 13;
    }
        if(num > 8192 && num <= 16384){
        bits = 14;
    }
        if(num > 16384 && num <= 32768){
        bits = 15;
    }
        if(num > 32768 && num <= 65536){
        bits = 16;
    }
    if(num > 65536 && num <= 131072){
        bits = 17;
    }
    if(num > 131072 && num <= 262144){
        bits = 18;
    }
    if(num > 262144 && num <= 524288){
        bits = 19;
    }
    if(num > 524288 && num <= 1048576){
        bits = 20;
    }
    if(num > 1048576 && num <= 2097152){
        bits = 21;
    }
    if(num > 2097152 && num <= 4194304){
        bits = 22;
    }
    if(num > 4194304 && num <= 8388608){
        bits = 23;
    }
    if(num > 8388608 && num <= 16777216	){
        bits = 24;
    }
    return bits;
}

/*
    Function to process a block of a file and compress it using lzwdr algorithm.
    @param block - block to be processed.
    @param blockSize - size of the block to be processed.
    @param dictionary - dictionary being used.
    @return - this function returns the output string after the block has been processed by the lzwdr algorithm.
*/
char* lzwdr(unsigned char *block, size_t blockSize, trieNode *dictionary) {
    char* outputString = malloc(blockSize * sizeof(char) * 4); // tamanho grande suficiente para armazenar a saída
    outputString[0] = '\0'; // inicializa a string de saída
    unsigned char* patternA = malloc(blockSize * sizeof(unsigned char));
    unsigned char* patternB = malloc(blockSize * sizeof(unsigned char));
    int sizeOfPatternA = 1;
    int sizeOfPatternB = 1;
    int code = 0;
    int index = 0;
    int i = 0;

    patternA[0] = block[index];
    index++; 
    while (index < blockSize ) {
        //Processing block to find the bigger patternB after patternA already in D.
        code = search(patternA, sizeOfPatternA, dictionary);
        patternB[0] = block[index];
        i = 1;
        while (index + 1 < blockSize && search(concat(patternB, sizeOfPatternB, &block[index + i], 1), sizeOfPatternB + 1, dictionary)) {
            unsigned char* aux = concat(patternB, sizeOfPatternB, &block[index + i], 1);
            sizeOfPatternB++;
            free(patternB);
            patternB = aux;
            i++;
        }

        //Send the code of patternA to the output. 
        outputString = output(code, outputString, strlen(outputString));
        compressedSize += numberOfBits(code);
        //Insert in the dictionary all the new patterns while the dictionary is not full
        int j = 0;
        int t = sizeOfTheDictionary - 1;

        while (j <= i && t < sizeOfTheDictionary) {
            unsigned char* newPattern = concat(patternA, sizeOfPatternA, patternB, j);
            if (search(newPattern, sizeOfPatternA + j, dictionary) == 0) {
                t = insert(newPattern, sizeOfPatternA + j, &dictionary);
                if (t < sizeOfTheDictionary) {
                    unsigned char* reversedPattern = reverse(newPattern, sizeOfPatternA + j);
                    if (search(reversedPattern, sizeOfPatternA + j, dictionary) == 0) {
                        t = insert(reversedPattern, sizeOfPatternA + j, &dictionary);
                    }
                    free(reversedPattern);
                }
            }
            free(newPattern);
            j++;
        }

        if (index + i >= blockSize) {
            output(search(patternB, sizeOfPatternB, dictionary), outputString, strlen(outputString));
            compressedSize += numberOfBits(search(patternB, sizeOfPatternB, dictionary));
            index += i;
        } else {
            index += i;
            free(patternA);
            patternA = malloc(sizeOfPatternB * sizeof(unsigned char));
            memcpy(patternA, patternB, sizeOfPatternB);
            sizeOfPatternA = sizeOfPatternB;
            sizeOfPatternB = 1;
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
    char* fileName;
    char* blockComparator;
    char* sizeAux;
    int numberOfBlocks = 0;
    long auxFileSize = 0;
    
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
    auxFileSize = fileSize;
    rewind(fileToCompress); 

    //Determining the size of the blocks being read from the file dependent on the value of argv[2].
    if(strcmp(blockComparator, "-86") == 0){
        blockSize = 88064;
    } else if(strcmp(blockComparator, "-32") == 0){
        blockSize = 32768;
    } else {
        blockSize = 65536;
    }

    if(strcmp(sizeAux, "-12") == 0){
        sizeOfTheDictionary = 4096;
    } else if(strcmp(sizeAux, "-24") == 0){
        sizeOfTheDictionary = 16777216;
    } else {
        sizeOfTheDictionary = 65536;
    }

    free(blockComparator);
    free(sizeAux);

    //populate the dictionary
    for(int i  = 0; i < ALPHABET_SIZE; i++) {
        unsigned char ind[1];
        ind[0] = (unsigned char)i;
        insert(ind, 1, &dictionary);
    }
    
    buffer = malloc(blockSize * sizeof(unsigned char));

    start = clock();
    outputFile = fopen("output.txt", "w");
    if(outputFile == NULL){
        fputs("Erro ao criar arquivo de saída", stderr);
        exit(1);
    }

    while (auxFileSize > 0) {
        bytesRead = fread(buffer, 1, blockSize, fileToCompress); 
        char* compressedData = lzwdr(buffer, bytesRead, dictionary);
        numberOfBlocks++;
        printf("Bloco %d tamanho do bloco %d\n", numberOfBlocks, bytesRead);
        fputs(compressedData, outputFile);
        free(compressedData);
        auxFileSize -= bytesRead;
    }

    end = clock();
    double duration = ((double)end - start) / CLOCKS_PER_SEC; //duration of the compression in seconds.
    printf("Numero blocos processados: %d\n", numberOfBlocks);
    printf("Tamanho de ficheiro processado: %d bytes\n", fileSize);
    printf("Tamanho de ficheiro comprimido: %d bytes\n", compressedSize/8);
    printf("Percentagem de compressao: %ld %%\n", 100 - ((compressedSize / 8) * 100) / fileSize);
    printf("Numero de codigos de padroes enviados para o output: %d\n", numberOfCodes);
    printf("Numero de vezes que o tamanho maximo do dicionario foi atingido: %d\n", dictionaryCap);
    printf("Duracao da compressao: %f segundos\n", duration);

    fclose(fileToCompress);
    fclose(outputFile);
    free(buffer);

    return 0;
}
