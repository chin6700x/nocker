/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Multi-Language Parity Benchmark - C Engine
 */

#include <stdio.h>
#include <stdint.h>

typedef struct {
    int64_t x;
    int64_t y;
    int64_t z;
} vector3_t;

int64_t compute_dot_product(const vector3_t *a, const vector3_t *b) {
    if (!a || !b) return 0;
    return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

int main(void) {
    vector3_t vec1 = { .x = 10, .y = 20, .z = 30 };
    vector3_t vec2 = { .x = 2,  .y = 3,  .z = 4  };

    int64_t dot = compute_dot_product(&vec1, &vec2);
    printf("⚡ [C-Runner] Vector Dot Product = %lld\n", dot);

    return 0;
}
