#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#define ALPHABET_SIZE 38

typedef struct trieNode {
    struct trieNode *children[ALPHABET_SIZE];
    int terminal;
    uint32_t ip_addr;
} trieNode;

static int char_to_index(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a'; // 0–25
    } else if (c >= '0' && c <= '9') {
        return 26 + (c - '0'); // 26–35
    } else if (c == '.') {
        return 36; // 36
    } else if (c == '-') {
        return 37; // 37
    } else {
        return -1; // invalid char
    }
}

trieNode* trie_create_node(void);
void trie_insert(trieNode* root, char *domain, uint32_t ip_addr);
int trie_lookup(trieNode* root, const char* domain, uint32_t *out_ip);
void trie_free(trieNode* root);