#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define HASH_MAP_ENTRIES    (UINT8_MAX + 1)
#define HASH_MAP_EMPTY      0

typedef void* ptr_t;

typedef struct {
    ptr_t e[HASH_MAP_ENTRIES];
} hash_map;

void hash_map_init(hash_map *m)
{
    int i;
    ptr_t *p = m->e;

    for (i = 0; i < HASH_MAP_ENTRIES; i++) {
        *p++ = HASH_MAP_EMPTY;
    }
}

uint8_t hash_map_function(uint16_t key)
{
    uint8_t result;
    uint16_t derived_key = key >> 2;
    
    result =  (derived_key & 0x7f);
    result ^= (derived_key >> 7);
    return result;
}

bool hash_map_insert(hash_map *m, uint16_t key, ptr_t val, ptr_t *evicted_val)
{
    bool evict = false;
    uint8_t index = hash_map_function(key);
    printf("%d -> %d\n", key, index);
    
    /* If there is a collision, evict. */
    if (m->e[index] != HASH_MAP_EMPTY) {
        evict = true;
        *evicted_val = m->e[index];
    }
    m->e[index] = val;

    return evict;
}

ptr_t hash_map_get(hash_map *m, uint16_t key)
{
    uint8_t index = hash_map_function(key);

    return m->e[index];
}

int main(int argc, char *argv[])
{
    uint16_t i;
    ptr_t p;
    int num_collisions = 0;
    hash_map m;

    srand(time(NULL));
    hash_map_init(&m);
    for (i = 0; i < 1024; i++) {
        uint16_t k = rand() & UINT16_MAX;
        if (hash_map_insert(&m, k, &p, &p)) {
            num_collisions++;
        }
    }
    printf("found %d collisions over %d entries (%lf\%)\n",
            num_collisions, 1024, 100.0*(double)num_collisions/(double)1024);
    return 0;
}
