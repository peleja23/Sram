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

/*
    Função para criar um novo nó na trie.
    @return newNode, retorna o novo nó.
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
    Função para adicionar um novo nó na trie.
    @param pattern - novo padrão que queremos inserir na função
    @param length - comprimento do padrão
    @param dictionary - dicionário sendo usado
    @return - essa função retorna um inteiro com o valor: 0 se o dicionário estiver cheio OU o valor do índice associado ao padrão inserido.
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
        return 0;
    } else {
        temporaryNode->indexOfPattern = indexOfPattern;
        printf("Padrão adicionado: ");
        for(int level = 0; level < length; level++){
             printf("%c", pattern[level]);
        }
        printf(" - %d\n", temporaryNode->indexOfPattern);
        ++indexOfPattern;
        return  temporaryNode->indexOfPattern;
    }
}

/*
    Função para buscar um nó na trie.
    @param pattern - novo padrão que queremos inserir na função
    @param length - comprimento do padrão
    @param dictionary - dicionário sendo usado
    @return - essa função retorna um inteiro com o valor: 0 se o padrão não existir OU o valor do índice associado ao padrão inserido.
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
    Função para adicionar um código de padrão à string de saída.
    @param code - o código a ser inserido na saída.
    @param string - string de saída atual.
    @param length - comprimento da string.
    @return - string de saída com o código adicionado.
*/
char* output(int code, char* string, int length){
    char codeString[12];
    sprintf(codeString, "%d,", code);
    strcat(string, codeString);
    //printf("Aqui: %s \n",string);
    return string;
}

/*
    Função para concatenar padrões.
    @param patternA - padrão A.
    @param lengthA - comprimento do padrão A.
    @param patternB - padrão B.
    @param lengthB - comprimento do padrão B.
    @return - essa função retorna a concatenação dos dois padrões de entrada.
*/
unsigned char* concat(unsigned char* patternA, int lengthA, unsigned char* patternB, int lengthB){
    unsigned char* concatenation = malloc((lengthA + lengthB) * sizeof(unsigned char));
    memcpy(concatenation, patternA, lengthA);
    memcpy(concatenation + lengthA, patternB, lengthB);
    return concatenation;
}

/*
    Função para inverter um array de unsigned char.
    @param originalArray - o array a ser invertido.
    @param length - comprimento do array a ser invertido.
    @return - essa função retorna um array de unsigned char com o valor invertido do originalArray.
*/
unsigned char* reverse(unsigned char *originalArray, int length) {
    unsigned char* invertedArray = malloc(length * sizeof(unsigned char));
    for (int i = 0; i < length; i++) {
        invertedArray[i] = originalArray[length - 1 - i];
    }
    return invertedArray;
}

/*
    Função para processar um bloco de um arquivo e compactá-lo usando o algoritmo LZWdR.
    @param block - bloco a ser processado.
    @param blockSize - tamanho do bloco a ser processado.
    @param dictionary - dicionário sendo usado.
    @return - essa função retorna a string de saída após o bloco ter sido processado pelo algoritmo LZWdR.
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
        // Processamento do bloco para encontrar o maior patternB após patternA já no dicionário.
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

        // Enviar o código de patternA para a saída.
        outputString = output(code, outputString, strlen(outputString));

        // Inserir no dicionário todos os novos padrões enquanto o dicionário não estiver cheio
        int j = 0;
        int t = sizeOfTheDictionary - 1;

        while (j <= sizeOfPatternB && t < sizeOfTheDictionary) {
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
    argv[1] - nome do arquivo que precisamos compactar.
    argv[2] - valor da string associado ao tamanho do bloco sendo utilizado.
    argv[3] - valor da string associado ao tamanho máximo do dicionário.
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
    
    // Alocação de memória + cópia de string para valores recebidos dos argumentos
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

    // Determinando o tamanho dos blocos sendo lidos do arquivo dependente do valor de argv[2].
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

    // Populando o dicionário
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

    while (fileSize > 0) {
        bytesRead = fread(buffer, 1, blockSize, fileToCompress); 
        char* compressedData = lzwdr(buffer, bytesRead, dictionary);
        fputs(compressedData, outputFile);
        free(compressedData);
        fileSize -= bytesRead;
    }

    end = clock();
    double duration = ((double)end - start) / CLOCKS_PER_SEC; // duração da compressão em segundos
    printf("Duração da compressão: %f segundos\n", duration);

    fclose(fileToCompress);
    fclose(outputFile);
    free(buffer);

    return 0;
}
