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

// Declaração de variáveis globais
long sizeOfTheDictionary = 0;
int indexOfPattern = 1;

typedef struct trieNode {
    struct trieNode *children[ALPHABET_SIZE];
    int indexOfPattern;
    int numberOfSearches;
} trieNode;

// Função para criar um novo nó na trie
trieNode *createNewNode() {
    trieNode *newNode = (trieNode *)malloc(sizeof(trieNode));

    if (newNode) {
        newNode->indexOfPattern = 0;
        memset(newNode->children, 0, sizeof(newNode->children));
    }

    return newNode;
}

// Função para adicionar um novo nó na trie
int insert(unsigned char *pattern, int length, trieNode **dictionary) {
    if (*dictionary == NULL) {
        *dictionary = createNewNode();
    }

    trieNode *temporaryNode = *dictionary;

    for (int level = 0; level < length; level++) {
        if (temporaryNode->children[pattern[level]] == NULL) {
            temporaryNode->children[pattern[level]] = createNewNode();
        }
        temporaryNode = temporaryNode->children[pattern[level]];
    }

    if (indexOfPattern >= sizeOfTheDictionary) {
        return 0;
    } else {
        temporaryNode->indexOfPattern = indexOfPattern++;
        printf("Padrao adicionado: %.*s - %d\n", length, pattern, temporaryNode->indexOfPattern);
        return temporaryNode->indexOfPattern;
    }
}

// Função para buscar um nó na trie
int search(unsigned char *pattern, int length, trieNode *dictionary) {
    trieNode *temporaryNode = dictionary;

    for (int i = 0; i < length; i++) {
        if (temporaryNode->children[pattern[i]] == NULL) {
            return 0;
        }
        temporaryNode = temporaryNode->children[pattern[i]];
    }
    return temporaryNode->indexOfPattern;
}

// Função para adicionar um código de padrão à string de saída
char *output(int code, char *string, int length) {
    char buffer[12];
    sprintf(buffer, "%d", code);
    strcat(string, buffer);
    printf("%d\n", code);
    return string;
}

// Função para concatenar padrões
unsigned char *concat(unsigned char *patternA, int lengthA, unsigned char *patternB, int lengthB) {
    unsigned char *concatenation = (unsigned char *)malloc((lengthA + lengthB) * sizeof(unsigned char));
    if (!concatenation) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    memcpy(concatenation, patternA, lengthA);
    memcpy(concatenation + lengthA, patternB, lengthB);

    printf("%.*s\n", lengthA + lengthB, concatenation);

    return concatenation;
}

// Função para inverter um array de unsigned char
unsigned char *reverse(unsigned char *originalArray, int length) {
    unsigned char *invertedArray = (unsigned char *)malloc(length * sizeof(unsigned char));
    if (!invertedArray) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < length; i++) {
        invertedArray[i] = originalArray[length - 1 - i];
    }

    return invertedArray;
}

// Função para processar um bloco de um arquivo e comprimi-lo usando o algoritmo lzwdr
char *lzwdr(unsigned char *block, size_t blockSize, trieNode *dictionary) {
    char *outputString = (char *)malloc(blockSize * sizeof(char) * 4);  // Ajustado para evitar possível estouro
    if (!outputString) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    outputString[0] = '\0';  // Inicializar string vazia

    unsigned char *patternA = (unsigned char *)malloc(blockSize * sizeof(unsigned char));
    unsigned char *patternB = (unsigned char *)malloc(blockSize * sizeof(unsigned char));
    if (!patternA || !patternB) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    int sizeOfPatternA = 0;
    int sizeOfPatternB = 0;
    int code = 0;
    int index = 0;

    patternA[sizeOfPatternA++] = block[index++];

    while (index < blockSize) {
        code = search(patternA, sizeOfPatternA, dictionary);
        patternB[sizeOfPatternB++] = block[index];

        while (search(concat(patternB, sizeOfPatternB, &block[index], 1), sizeOfPatternB + 1, dictionary) != 0 && (index) < blockSize) {
            patternB = (unsigned char *)realloc(patternB, (sizeOfPatternB + 1) * sizeof(unsigned char));
            if (!patternB) {
                fprintf(stderr, "Memory allocation error\n");
                exit(EXIT_FAILURE);
            }
            patternB[sizeOfPatternB++] = block[index++];
        }

        outputString = output(code, outputString, strlen(outputString));

        for (int j = 0; j <= sizeOfPatternB && indexOfPattern < sizeOfTheDictionary; j++) {
            unsigned char *newPattern = concat(patternA, sizeOfPatternA, &patternB[j], 1);
            insert(newPattern, sizeOfPatternA + 1, &dictionary);
            free(newPattern);

            unsigned char *reversedPattern = reverse(patternA, sizeOfPatternA + 1);
            insert(reversedPattern, sizeOfPatternA + 1, &dictionary);
            free(reversedPattern);
        }

        if (index >= blockSize) {
            output(search(patternB, sizeOfPatternB, dictionary), outputString, strlen(outputString));
        } else {
            index += sizeOfPatternB;
            memcpy(patternA, patternB, sizeOfPatternB);
            sizeOfPatternA = sizeOfPatternB;
            sizeOfPatternB = 0;
        }
    }

    free(patternA);
    free(patternB);
    return outputString;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <filename> <blocksize> <dicsize>\n", argv[0]);
        return EXIT_FAILURE;
    }

    trieNode *dictionary = NULL;
    FILE *fileToCompress;
    unsigned char *buffer;
    size_t blockSize;
    long fileSize;
    size_t bytesRead;
    clock_t start, end;
    char *fileName = argv[1];
    char *blockComparator = argv[2];
    char *sizeAux = argv[3];

    fileToCompress = fopen(fileName, "rb");
    if (!fileToCompress) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    fseek(fileToCompress, 0, SEEK_END);
    fileSize = ftell(fileToCompress);
    rewind(fileToCompress);

    blockSize = (strcmp(blockComparator, "-86") == 0) ? 88064 :
                (strcmp(blockComparator, "-32") == 0) ? 32768 : 65536;

    sizeOfTheDictionary = (strcmp(sizeAux, "-12") == 0) ? 4096 :
                          (strcmp(sizeAux, "-24") == 0) ? 16777216 : 65536;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        unsigned char ind[1] = {(unsigned char)i};
        insert(ind, 1, &dictionary);
    }

    buffer = (unsigned char *)malloc(blockSize * sizeof(unsigned char));
    if (!buffer) {
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    start = clock();
    while (fileSize > 0) {
        bytesRead = fread(buffer, 1, blockSize, fileToCompress);
        if (bytesRead == 0) {
            break;
        }

        lzwdr(buffer, bytesRead, dictionary);
        fileSize -= bytesRead;
    }
    end = clock();
    double duration = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("Compression completed in %.2f seconds\n", duration);

    fclose(fileToCompress);
    free(buffer);

    return 0;
}
