#include <stdio.h>
#include <stdlib.h>

#define ALPHABET_SIZE 256

int main() {
    FILE *file;
    char *charbuffer;
    long file_size;
    size_t result;
    int block_size = 512;

    file = fopen("arquivo.txt", "rb");
    if (file == NULL) {
        fputs("Erro ao abrir arquivo", stderr);
        exit(1);
    }
    // Obter tamanho do arquivo
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    // Alocar memória para o buffer
    charbuffer = (char) malloc(sizeof( block_size));
 
    // Ler o arquivo em blocos
    while (file_size > 0) {
        result = fread(charbuffer, 1, block_size, file);
        if (result != block_size && !feof(file)) {
            fputs("Erro ao ler arquivo", stderr);
            exit(1);
        }

        // Processar bloco de dados
        // ...

        file_size -= result;
    }

    fclose(file);
    free(charbuffer);

    return 0;
}