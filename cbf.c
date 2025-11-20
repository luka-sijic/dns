#include "cbf.h"


CBF* createCBF(double n, double p) {
    CBF* c = malloc(sizeof(CBF));
    c->m = (int)ceil(-(n * log(p)) / (log(2)) * (log(2)));
    c->k = (int)ceil(((double)c->m / n) * log(2));
    c->arr = calloc(c->m, sizeof(int));
    return c;
}

void insert(CBF* c, const char* domain) {
    for (int i = 0;i < c->k;i++) {
        size_t idx = cbfIndex(domain, i, c->m);
        c->arr[idx]++;
    }
}

int possiblyExists(CBF *c, const char *domain) {
    for (int i = 0;i < c->k;i++) {
        size_t idx = cbfIndex(domain, i, c->m);
        if (c->arr[idx] == 0) return 0;
    }
    return 1;
}

void freeCBF(CBF *c) {
    if (!c) return;
    free(c->arr);
    free(c);
}


int main() {
    CBF* c = createCBF(100, .001);
    insert(c, "example.com");
    printf("%d\n", possiblyExists(c, "example.com"));
    printf("%d\n", possiblyExists(c, "cnn.com"));
    freeCBF(c);
    return 0;
}