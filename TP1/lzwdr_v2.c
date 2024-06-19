#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHAR_SET_SIZE 256

// Trie node structure
typedef struct TrieNode {
    struct TrieNode *children[CHAR_SET_SIZE];
    int isEndOfPattern;
} TrieNode;

// Função para criar um novo nó da trie
TrieNode* createNode() {
    TrieNode* node = (TrieNode*)malloc(sizeof(TrieNode));
    node->isEndOfPattern = 0;
    for (int i = 0; i < CHAR_SET_SIZE; i++) {
        node->children[i] = NULL;
    }
    return node;
}

// Função para inserir um padrão na trie
void insertPattern(TrieNode *root, const char *pattern) {
    TrieNode *node = root;
    int len = strlen(pattern);
    for (int i = 0; i < len; i++) {
        unsigned char index = (unsigned char)pattern[i]; // Usa unsigned char para garantir valores dentro de [0, 255]
        if (!node->children[index]) {
            node->children[index] = createNode();
        }
        node = node->children[index];
    }
    node->isEndOfPattern = 1;
}

// Função para buscar um padrão na trie
int searchPattern(TrieNode *root, const char *pattern) {
    TrieNode *node = root;
    int len = strlen(pattern);
    for (int i = 0; i < len; i++) {
        unsigned char index = (unsigned char)pattern[i]; // Usa unsigned char para garantir valores dentro de [0, 255]
        if (!node->children[index]) {
            return 0;
        }
        node = node->children[index];
    }
    return node != NULL && node->isEndOfPattern;
}

// Função para adicionar um padrão e seu reverso na trie
void addPatternAndReverse(TrieNode *root, const char *pattern) {
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
    TrieNode *root = createNode();

    // Adiciona caracteres ASCII na trie
  
    for (int i = 0; i < CHAR_SET_SIZE; i++) {
        char character = (char)i;
        char pattern[2] = { character, '\0' };
        insertPattern(root, pattern);
        printf("Initial trie characters added: ");
        printf("%c ", character);
         printf("\n");
    }
   

    int len = strlen(sequence);

    // Adiciona todos os padrões e seus reversos na trie
    for (int i = 0; i < len; i++) {
        for (int j = i + 1; j <= len; j++) {
            char *pattern = (char *)malloc((j - i + 1) * sizeof(char));
            strncpy(pattern, sequence + i, j - i);
            pattern[j - i] = '\0';

            addPatternAndReverse(root, pattern);
            free(pattern);
        }
    }

    // Libera memória usada pela trie
    void freeTrie(TrieNode* node) {
        for (int i = 0; i < CHAR_SET_SIZE; i++) {
            if (node->children[i]) {
                freeTrie(node->children[i]);
            }
        }
        free(node);
    }

    freeTrie(root);
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

    return 0;
}
