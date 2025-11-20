/*
#include <arpa/inet.h>
#include <stdio.h>
#include "trie.h"


int main() {
    trieNode* t = trie_create_node();

    char data[256];
    FILE *ptr = fopen("hosts.txt", "r");
    while (fgets(data, sizeof(data), ptr)) {
        char first[128], second[128];
        if (sscanf(data, "%127s %127s", first, second) == 2) {
            uint32_t ip;
            inet_pton(AF_INET, second, &ip);
            trie_insert(t, first, ip);
            printf("first = %s, second = %s\n", first, second);
        } else {
            fprintf(stderr, "failed to parse line: %s", data);
        }
    }  

    const char *query = "google.com";
    uint32_t ip;
    if (trie_lookup(t, query, &ip)) {
        char buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &ip, buf, sizeof buf)) {
            printf("%s -> %s\n", query, buf);
        }
    }

    fclose(ptr);
    return 0;
}*/