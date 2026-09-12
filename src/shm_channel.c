#include "shm_channel.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static int mapping_is_valid(const shm_mapping_t *mapping)
{
    return mapping != NULL && mapping->mapping != NULL && mapping->mapped_size > 0u;
}

bus_status_t shm_mapping_create(const char *name, size_t size, shm_mapping_t *out)
{
    int fd = -1;
    void *mapping = NULL;
    char shm_path[256];

    if (name == NULL || out == NULL || size == 0u) {
        return BUS_ERR_NULL;
    }

    memset(out, 0, sizeof(*out));
    (void)snprintf(shm_path, sizeof(shm_path), "/%s", name);

    shm_unlink(shm_path);
    fd = shm_open(shm_path, O_CREAT | O_RDWR, 0600);
    if (fd < 0) {
        return BUS_ERR_SIZE;
    }
    if (ftruncate(fd, (off_t)size) != 0) {
        close(fd);
        shm_unlink(shm_path);
        return BUS_ERR_SIZE;
    }

    mapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        close(fd);
        shm_unlink(shm_path);
        return BUS_ERR_SIZE;
    }

    memset(mapping, 0, size);
    out->fd = fd;
    out->mapped_size = size;
    out->mapping = mapping;
    return BUS_OK;
}

bus_status_t shm_mapping_open(const char *name, size_t size, shm_mapping_t *out)
{
    int fd = -1;
    void *mapping = NULL;
    char shm_path[256];

    if (name == NULL || out == NULL || size == 0u) {
        return BUS_ERR_NULL;
    }

    memset(out, 0, sizeof(*out));
    (void)snprintf(shm_path, sizeof(shm_path), "/%s", name);

    fd = shm_open(shm_path, O_RDWR, 0600);
    if (fd < 0) {
        return BUS_ERR_SIZE;
    }

    mapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        close(fd);
        return BUS_ERR_SIZE;
    }

    out->fd = fd;
    out->mapped_size = size;
    out->mapping = mapping;
    return BUS_OK;
}

void shm_mapping_close(shm_mapping_t *mapping)
{
    if (!mapping_is_valid(mapping)) {
        return;
    }

    munmap(mapping->mapping, mapping->mapped_size);
    close(mapping->fd);
    mapping->mapping = NULL;
    mapping->mapped_size = 0u;
    mapping->fd = -1;
}

void shm_record_publish_broken(shm_record_t *record, uint32_t seq, uint32_t value_a, uint32_t value_b)
{
    if (record == NULL) {
        return;
    }

    /* Publish sequence first to make torn reads observable in the demo. */
    record->seq = seq;
    record->value_a = value_a;
    record->value_b = value_b;
}

int shm_record_consume_broken(const shm_record_t *record, uint32_t *seq, uint32_t *value_a, uint32_t *value_b)
{
    uint32_t seq_before;
    uint32_t seq_after;

    if (record == NULL || seq == NULL || value_a == NULL || value_b == NULL) {
        return 0;
    }

    seq_before = record->seq;
    *value_a = record->value_a;
    *value_b = record->value_b;
    seq_after = record->seq;

    *seq = seq_before;
    return seq_before != seq_after;
}

void shm_record_publish(shm_record_t *record, uint32_t seq, uint32_t value_a, uint32_t value_b)
{
    if (record == NULL) {
        return;
    }

    record->value_a = value_a;
    record->value_b = value_b;
    __atomic_store_n(&record->seq, seq, __ATOMIC_RELEASE);
}

int shm_record_consume(const shm_record_t *record, uint32_t expected_seq, uint32_t *value_a, uint32_t *value_b)
{
    uint32_t seq_before;
    uint32_t seq_after;
    uint32_t local_a;
    uint32_t local_b;

    if (record == NULL || value_a == NULL || value_b == NULL) {
        return 0;
    }

    seq_before = __atomic_load_n(&record->seq, __ATOMIC_ACQUIRE);
    local_a = record->value_a;
    local_b = record->value_b;
    seq_after = __atomic_load_n(&record->seq, __ATOMIC_ACQUIRE);

    if (seq_before != seq_after || seq_before != expected_seq) {
        return 1;
    }

    *value_a = local_a;
    *value_b = local_b;
    return 0;
}

bus_status_t shm_channel_publish(shm_channel_t *channel, const shm_record_t *record)
{
    uint32_t index;
    uint32_t next;

    if (channel == NULL || record == NULL) {
        return BUS_ERR_NULL;
    }

    index = __atomic_load_n(&channel->write_index, __ATOMIC_RELAXED);
    channel->slots[index % SHM_SLOT_COUNT] = *record;
    __atomic_store_n(&channel->slot_seq[index % SHM_SLOT_COUNT], record->seq, __ATOMIC_RELEASE);

    next = index + 1u;
    __atomic_store_n(&channel->write_index, next, __ATOMIC_RELEASE);
    return BUS_OK;
}

bus_status_t shm_channel_try_consume(shm_channel_t *channel, shm_record_t *out_record)
{
    uint32_t read_index;
    uint32_t write_index;
    uint32_t slot_index;
    uint32_t seq_before;
    uint32_t seq_after;
    shm_record_t snapshot;

    if (channel == NULL || out_record == NULL) {
        return BUS_ERR_NULL;
    }

    read_index = __atomic_load_n(&channel->read_index, __ATOMIC_RELAXED);
    write_index = __atomic_load_n(&channel->write_index, __ATOMIC_ACQUIRE);
    if (read_index >= write_index) {
        return BUS_ERR_SIZE;
    }

    slot_index = read_index % SHM_SLOT_COUNT;
    seq_before = __atomic_load_n(&channel->slot_seq[slot_index], __ATOMIC_ACQUIRE);
    snapshot = channel->slots[slot_index];
    seq_after = __atomic_load_n(&channel->slot_seq[slot_index], __ATOMIC_ACQUIRE);

    if (seq_before != seq_after || seq_before != snapshot.seq) {
        return BUS_ERR_CHECKSUM;
    }

    *out_record = snapshot;
    __atomic_store_n(&channel->read_index, read_index + 1u, __ATOMIC_RELEASE);
    return BUS_OK;
}
