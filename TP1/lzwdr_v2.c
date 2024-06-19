#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHAR_SET_SIZE 128  // Considera todos os caracteres ASCII imprimíveis

// Estrutura para o nó da trie
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
        int index = (int)pattern[i];
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
        int index = (int)pattern[i];
        if (!node->children[index]) {
            return 0;
        }
        node = node->children[index];
    }
    return node != NULL && node->isEndOfPattern;
}

// Função para adicionar um padrão e seu inverso na trie
void addPatternAndReverse(TrieNode *root, const char *pattern) {
    if (!searchPattern(root, pattern)) {
        insertPattern(root, pattern);
        printf("Padrão adicionado: %s\n", pattern);
    }
    
    int len = strlen(pattern);
    char *reverse_pattern = (char *)malloc(len + 1);
    for (int i = 0; i < len; i++) {
        reverse_pattern[i] = pattern[len - i - 1];
    }
    reverse_pattern[len] = '\0';
    
    if (!searchPattern(root, reverse_pattern)) {
        insertPattern(root, reverse_pattern);
        printf("Padrão inverso adicionado: %s\n", reverse_pattern);
    }
    
    free(reverse_pattern);
}

// Função de compressão que identifica e armazena padrões e padrões inversos utilizando tries
void compress(const char *sequence) {
    TrieNode *root = createNode();
    int len = strlen(sequence);

    for (int i = 0; i < len; i++) {
        for (int j = i + 1; j <= len; j++) {
            char *pattern = (char *)malloc((j - i + 1) * sizeof(char));
            strncpy(pattern, sequence + i, j - i);
            pattern[j - i] = '\0';

            addPatternAndReverse(root, pattern);
            free(pattern);
        }
    }

    // Liberação da memória da trie
    // Função recursiva para liberar a memória
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

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <arquivo de entrada> <param1> <param2>\n", argv[0]);
        return 1;
    }

    const char *inputFileName = argv[1];
    FILE *inputFile = fopen(inputFileName, "r");
    if (inputFile == NULL) {
        perror("Erro ao abrir o arquivo de entrada");
        return 1;
    }

    fseek(inputFile, 0, SEEK_END);
    long inputFileSize = ftell(inputFile);
    fseek(inputFile, 0, SEEK_SET);

    char *input = (char *)malloc((inputFileSize + 1) * sizeof(char));
    fread(input, sizeof(char), inputFileSize, inputFile);
    input[inputFileSize] = '\0';

    fclose(inputFile);

    compress(input);

    free(input);
    return 0;
}
