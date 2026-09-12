#ifndef SHM_CHANNEL_H
#define SHM_CHANNEL_H

#include "message.h"

#include <stddef.h>
#include <stdint.h>

#define SHM_CHANNEL_NAME     "bus_sim_channel"
#define SHM_SLOT_COUNT       64u
#define SHM_SLOT_PAYLOAD     240u

typedef struct {
    uint32_t seq;
    uint32_t value_a;
    uint32_t value_b;
} shm_record_t;

typedef struct {
    uint32_t write_index;
    uint32_t read_index;
    shm_record_t slots[SHM_SLOT_COUNT];
} shm_channel_broken_t;

typedef struct {
    uint32_t write_index;
    uint32_t read_index;
    uint32_t slot_seq[SHM_SLOT_COUNT];
    shm_record_t slots[SHM_SLOT_COUNT];
} shm_channel_t;

typedef struct {
    int fd;
    size_t mapped_size;
    void *mapping;
} shm_mapping_t;

bus_status_t shm_mapping_create(const char *name, size_t size, shm_mapping_t *out);

bus_status_t shm_mapping_open(const char *name, size_t size, shm_mapping_t *out);

void shm_mapping_close(shm_mapping_t *mapping);

void shm_record_publish_broken(shm_record_t *record, uint32_t seq, uint32_t value_a, uint32_t value_b);

int shm_record_consume_broken(const shm_record_t *record, uint32_t *seq, uint32_t *value_a, uint32_t *value_b);

void shm_record_publish(shm_record_t *record, uint32_t seq, uint32_t value_a, uint32_t value_b);

int shm_record_consume(const shm_record_t *record, uint32_t expected_seq, uint32_t *value_a, uint32_t *value_b);

bus_status_t shm_channel_publish(shm_channel_t *channel, const shm_record_t *record);

bus_status_t shm_channel_try_consume(shm_channel_t *channel, shm_record_t *out_record);

#endif /* SHM_CHANNEL_H */
