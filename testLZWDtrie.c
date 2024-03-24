#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define ALPHABET_SIZE 256

//Declaration of global variables
int sizeOfTheDictionary = 0;
int inxdexOfPattern = 0;
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
    @return - this function returns an integer with the value: -1 if the dictionary is full OR the value of index associated with the inserted pattern.
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
        return -1;
    } else {
        temporaryNode->indexOfPattern = inxdexOfPattern;
        ++inxdexOfPattern;
        //printf("Indice do padrao: %d\n",temporaryNode->indexOfPattern);
        return  temporaryNode->indexOfPattern;
    }
}

/*
    Function to search for a node in the trie.
    @param dictionary - dictionary being used
    @param pattern - new pattern that we want to insert in the function
    @return - this function returns an integer with the value: -1 if the pattern doesn't exists OR the value of index associated with the inserted pattern.
*/
int searchInTrie(trieNode *dictionary, char *pattern){
    unsigned char *text = (unsigned char *)pattern;
    int lenght = strlen(pattern);

    trieNode * temporaryNode = dictionary;

    for(int i = 0; i < lenght ; i++){
        if(temporaryNode->children[text[i]] == NULL){
            return -1;
        }

        unsigned char newText = text[i];
        temporaryNode = temporaryNode->children[newText];
    }
    return temporaryNode->indexOfPattern;
}

int main(){


    trieNode * root = createNewNode();
    char* TestString = "ABCDABCDDDBABDCDABABSBCDABBCCA";

    /* Insert and Search tests
    char* text = "ab";
    printf("%d\n", insertNode(&root, "aa"));
    insertNode(&root, "aaa");
    insertNode(&root, "ab");
    insertNode(&root, "abc");
    insertNode(&root, "abcc");
    insertNode(&root, "também");

    printf("Search 1: %d\n", searchInTrie(root, "aa"));  
    printf("Search 2: %d\n", searchInTrie(root, "abcc"));  
    printf("Search 3: %d\n", searchInTrie(root, "abc"));  
    printf("Search 4: %d\n", searchInTrie(root, "bb"));  
    printf("Search 5: %d\n", searchInTrie(root, "também"));  
    printf("Search 6: %d\n", searchInTrie(root, text));  
    */
    bool continueSearch = true;
    char *prefixA = (char *)malloc(sizeof(TestString));
    char *prefixB = (char *)malloc(sizeof(TestString));
    char * newChar = (char *)malloc(sizeof(TestString));    
    //memcpy(prefix, TestString, 1);

    //prefix[0] = TestString[0];
    for(int i=0; i<strlen(TestString); i++){
        char* aux= (char *)malloc(sizeof(TestString));
        aux[0]=TestString[i];
        aux[1]='\0';
        int comparator = searchInTrie(root, aux);
        if(comparator == -1){
            insertNode(&root, aux);
        }
        
    }

    printf("%d\n", strlen(TestString));

    int index = 2;

    prefixA[0] = TestString[0];
    prefixA[1] = '\0';

    int i = 0;

    char * auxA = (char *)malloc(sizeof(TestString));
    char * auxB = (char *)malloc(sizeof(TestString)); 

    prefixB[0] = TestString[1];
    prefixB[1] = '\0';      

 while (index <strlen(TestString))
    {
        
        strcpy(auxA, prefixA);
        strcpy(auxB, prefixB);
        while (continueSearch)
        {
            newChar[0] = TestString[index+i];
            newChar[1] = '\0';
            
            int comp = searchInTrie(root, auxB);
            
            int currentInd = index +i;
            if (comp != -1){                

                insertNode(&root, strcat(auxA, auxB));
                strcat(auxB, newChar);
                printf("\n %s\n", auxB);
                i = i+1;
                
            }else{
                continueSearch = false;
                i = i+1;
                index = currentInd+i;
                memcpy(prefixA, prefixB, i-1);
                prefixA[i] = "\0";
                memcpy(prefixB, newChar, i-1);
                prefixB[i] = "\0";

                //strcpy(prefixB, auxB);

                printf("Prefixo A: %s\t Prefixo B: %s\n", prefixA, prefixB);
                printf("current index: %d\n", currentInd);
                i = 0;
            }
            
        }
        
    continueSearch = true;
        
    }
    
   
    //printf("%d\n", strlen(prefixA));

    //printf("%s\n", prefixA);1

    //printf("Search 1: %d\n", searchInTrie(root, prefixA));  
}
