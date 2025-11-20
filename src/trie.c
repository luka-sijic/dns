#include "trie.h"

trieNode* trie_create_node() {
    trieNode* node = malloc(sizeof(trieNode));
    for (int i = 0;i < ALPHABET_SIZE;++i) node->children[i] = NULL;
    node->terminal = 0;
    node->ip_addr = 0;
    return node;
}


void trie_insert(trieNode* root, char *domain, uint32_t ip_addr) {
    trieNode *cur = root;
    for (size_t i = 0;domain[i] != '\0';i++) {
        char c = tolower((unsigned char)domain[i]);
        int idx = char_to_index(c);
        if (!cur->children[idx]) cur->children[idx] = trie_create_node();
        cur = cur->children[idx];
    }
    cur->terminal = 1;
    cur->ip_addr = ip_addr;
}

int trie_lookup(trieNode* root, const char* domain, uint32_t *out_ip) {
    trieNode* cur = root;
    for (size_t i = 0;domain[i] != '\0';i++) {
        char c = tolower((unsigned char)domain[i]);
        int idx = char_to_index(c);
        if (idx < 0) return 0;
        if (!cur->children[idx]) return 0;
        cur = cur->children[idx];
    }
    if (cur->terminal) {
        if (out_ip) {
            *out_ip = cur->ip_addr;
        }
        return 1;
    }
    return 0;
}

void trie_free(trieNode* root) {
    for (int i = 0;i < ALPHABET_SIZE;i++) trie_free(root->children[i]);
    free(root);
}


/*
int main() {
    trieNode *t = trie_create_node();

    uint32_t ip1, ip2;
    inet_pton(AF_INET, "192.168.0.158", &ip1);
    inet_pton(AF_INET, "192.168.0.172", &ip2);

    trie_insert(t, "google.com", ip1);
    trie_insert(t, "cnn.com", ip2);

    const char *query = "google.com";
    uint32_t ip;
    if (trie_lookup(t, query, &ip)) {
        char buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &ip, buf, sizeof buf)) {
            printf("%s -> %s\n", query, buf);
        }
    }
    return 0;
}*/