#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ALPHABET_SIZE 256

typedef struct trieNode {
    struct trieNode *children[ALPHABET_SIZE]; 
    bool isEndOfPattern; // true if the node is the end of a pattern
} trieNode;

trieNode *createNewNode(){
    trieNode *newNode = malloc(sizeof *newNode);
    
    if(newNode){
        newNode->isEndOfPattern = false;
        
        for(int i = 0; i < ALPHABET_SIZE; i++){
            newNode->children[i] = NULL;
        }
    }
    
    return newNode;
}

bool insertNode(trieNode **root, char *signedText){

    if(*root == NULL){
        *root = createNewNode();
    }

    unsigned char *text = (unsigned char *)signedText;
    trieNode *temporaryNode = *root;
    int lenghtText =  strlen(signedText);

    for(int level = 0; level < lenghtText; level++){
        if(temporaryNode->children[text[level]] == NULL){
            temporaryNode->children[text[level]] = createNewNode();
        }
        temporaryNode = temporaryNode->children[text[level]];
    }

    if(temporaryNode->isEndOfPattern){
        return false;
    } else {
        temporaryNode->isEndOfPattern = true;
        return  true;
    }
}

bool searchInTrie(trieNode *root, char *signedText){
    unsigned char *text = (unsigned char *)signedText;
    int lenght = strlen(signedText);

    trieNode * temporaryNode = root;

    for(int i = 0; i < lenght ; i++){
        if(temporaryNode->children[text[i]] == NULL){
            return false;
        }

        unsigned char newText = text[i];
        temporaryNode = temporaryNode->children[newText];
    }

    return temporaryNode->isEndOfPattern;
}

 recursiveTriePrint(trieNode *currentNode, char *previousCharacters, int lenght){
    unsigned char newPreviousCharacters[lenght+2];
    memcpy(newPreviousCharacters, previousCharacters, lenght);
    newPreviousCharacters[lenght+1]=0;

    if(currentNode->isEndOfPattern){
        printf("Code: %s\n", previousCharacters);
    }

    for(int i=0; i < ALPHABET_SIZE; i++){
        if(currentNode->children[i] != NULL){
            newPreviousCharacters[lenght] = i;
            recursiveTriePrint(currentNode->children[i], newPreviousCharacters, lenght +1);
        }
    }
    
}

void printTrie(trieNode *root){
    if(root == NULL){
        printf("No nodes in the trie\n");
        return;
    }
    recursiveTriePrint(root, NULL, 0);
}


int main(){


    trieNode * root = NULL;

    printf("%s\n", insertNode(&root, "aa")?"true":"false");
    printf("%s\n", insertNode(&root, "aa")?"true":"false");
    insertNode(&root, "aaa");
    insertNode(&root, "ab");
    insertNode(&root, "abc");
    insertNode(&root, "abcc");


    printTrie(root);

}