#include "cbf.h"


CBF* cbf_create(double n, double p) {
    CBF* c = malloc(sizeof(CBF));
    c->m = (int)ceil(-(n * log(p)) / (log(2)) * (log(2)));
    c->k = (int)ceil(((double)c->m / n) * log(2));
    c->arr = calloc(c->m, sizeof(int));
    return c;
}

void cbf_insert(CBF* c, const char* domain) {
    for (int i = 0;i < c->k;i++) {
        size_t idx = cbfIndex(domain, i, c->m);
        c->arr[idx]++;
    }
}

int cbf_possibly_exists(CBF *c, const char *domain) {
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

/*
int main() {
    CBF* c = cbf_create(100, .001);
    cbf_insert(c, "example.com");
    printf("%d\n", cbf_possible_exists(c, "example.com"));
    printf("%d\n", cbf_possible_exists(c, "cnn.com"));
    freeCBF(c);
    return 0;
}*/