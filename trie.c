#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ALPHABET_SIZE 38

typedef struct trieNode {
    struct trieNode *children[ALPHABET_SIZE];
    int terminal;
    uint32_t ip_addr;
} trieNode;

trieNode* createTrieNode() {
    trieNode* t = malloc(sizeof(trieNode));
    for (int i = 0;i < 256;++i) t->children[i] = NULL;
    t->terminal = 0;
    t->ip_addr = NULL;
    return t;
}

void insert(trieNode* t, char *domain, char *ip_addr) {
    unsigned char *text = (unsigned char*)domain;
}

int main() {
    trieNode *t = createTrieNode();
    return 0;
}