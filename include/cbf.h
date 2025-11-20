#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

typedef struct CBF {
    uint32_t *arr;
    size_t m;
    int k;
} CBF;

CBF* cbf_create(double n, double p);

void cbf_insert(CBF* c, const char* domain);

int cbf_possibly_exists(CBF* c, const char* domain);

static inline uint64_t fnv1a64(const void *data, size_t len) {
    const unsigned char *p = (const unsigned char *)data;
    uint64_t h = 1469598103934665603ULL;      // offset basis
    for (size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 1099511628211ULL;               // FNV prime
    }
    return h;
}

static inline size_t cbfIndex(const char *domain, int i, size_t m) {
    size_t len = strlen(domain);
    uint64_t h  = fnv1a64(domain, len);

    uint64_t h1 = h;
    uint64_t h2 = (h >> 33) | (h << 31); // simple rotate to get a 2nd value

    uint64_t combined = h1 + (uint64_t)i * h2;
    return (size_t)(combined % m);
}