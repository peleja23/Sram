#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHAR_SET_SIZE 6000

// Trie node structure
typedef struct TrieNode {
    struct TrieNode *children[CHAR_SET_SIZE];
    int isEndOfPattern;
} TrieNode;

// Variável global para a raiz da Trie
TrieNode *root = NULL;

// Função para criar um novo nó da trie
TrieNode* createNode() {
    TrieNode* node = (TrieNode*)malloc(sizeof(TrieNode));
    node->isEndOfPattern = 0;
    for (int i = 0; i < CHAR_SET_SIZE; i++) {
        node->children[i] = NULL;
    }
    return node;
}

// Função para inicializar a Trie com caracteres ASCII
void initializeTrie() {
    if (root == NULL) {
        root = createNode();
       
        for (int i = 0; i < 256; i++) {
            char character = (char)i;
            char pattern[2] = { character, '\0' };
            insertPattern(root, pattern);
            printf("Initial trie characters added: ");
            printf("%c \n", character);
        }
    }
}

// Função para inserir um padrão na trie
void insertPattern(TrieNode *node, const char *pattern) {
    TrieNode *current = node;
    int len = strlen(pattern);
    for (int i = 0; i < len; i++) {
        unsigned char index = (unsigned char)pattern[i];
        if (!current->children[index]) {
            current->children[index] = createNode();
        }
        current = current->children[index];
    }
    //ad
    current->isEndOfPattern = 1;
}

// Função para buscar um padrão na trie
int searchPattern(TrieNode *node, const char *pattern) {
    TrieNode *current = node;
    int len = strlen(pattern);
    for (int i = 0; i < len; i++) {
        unsigned char index = (unsigned char)pattern[i];
        if (!current->children[index]) {
            return 0;
        }
        current = current->children[index];
    }
    return current != NULL && current->isEndOfPattern;
}

// Função para adicionar um padrão e seu reverso na trie
void addPatternAndReverse(const char *pattern) {
    if (!searchPattern(root, pattern)) {
        insertPattern(root, pattern);
        printf("Pattern added: %s\n", pattern);
    }
    
    int len = strlen(pattern);
    char *reverse_pattern = (char *)malloc(len + 1);
    for (int i = 0; i < len; i++) {
        reverse_pattern[i] = pattern[len - i - 1];
    }
    reverse_pattern[len] = '\0';
    
    if (!searchPattern(root, reverse_pattern)) {
        insertPattern(root, reverse_pattern);
        printf("Reverse pattern added: %s\n", reverse_pattern);
    }
    
    free(reverse_pattern);
}

// Função para comprimir uma sequência usando trie para padrões e seus reversos
void compress(const char *sequence) {
    initializeTrie();

    int len = strlen(sequence);
    for (int i = 0; i < len; i++) {
        for (int j = i + 1; j <= len; j++) {
            char *pattern = (char *)malloc((j - i + 1) * sizeof(char));
            strncpy(pattern, sequence + i, j - i);
            pattern[j - i] = '\0';

            addPatternAndReverse(pattern);
            free(pattern);
        }
    }
}

// Função para processar arquivo em pedaços e comprimir cada pedaço
void processFileInChunks(const char *filename, size_t chunkSize) {
    FILE *inputFile = fopen(filename, "r");
    if (inputFile == NULL) {
        perror("Error opening input file");
        exit(1);
    }

    fseek(inputFile, 0, SEEK_END);
    long inputFileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    char *buffer = (char *)malloc(chunkSize * sizeof(char));
    size_t bytesRead = 0;

    while ((bytesRead = fread(buffer, sizeof(char), chunkSize, inputFile)) > 0) {
        buffer[bytesRead] = '\0';  // Termina com nulo o buffer
        compress(buffer);
    }

    free(buffer);
    fclose(inputFile);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input file> <param1> <param2>\n", argv[0]);
        return 1;
    }

    const char *inputFileName = argv[1];
    size_t chunkSize = 65536;

    processFileInChunks(inputFileName, chunkSize);

    // Liberar memória da Trie não é necessário aqui, conforme requisito de não limpar a Trie

    return 0;
}
