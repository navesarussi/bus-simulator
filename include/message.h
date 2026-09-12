#ifndef BUS_MESSAGE_H
#define BUS_MESSAGE_H

#include <stddef.h>
#include <stdint.h>

#define BUS_SYNC_WORD        0xA5A5u
#define BUS_MAX_DATA_WORDS   32u
#define BUS_HEADER_SIZE      6u   /* sync(2) + rt(1) + sub(1) + count(1) + flags(1) */
#define BUS_CHECKSUM_SIZE    2u
#define BUS_MAX_MESSAGE_SIZE (BUS_HEADER_SIZE + (BUS_MAX_DATA_WORDS * 2u) + BUS_CHECKSUM_SIZE)

typedef enum {
    BUS_OK = 0,
    BUS_ERR_NULL = -1,
    BUS_ERR_SIZE = -2,
    BUS_ERR_SYNC = -3,
    BUS_ERR_WORD_COUNT = -4,
    BUS_ERR_CHECKSUM = -5,
    BUS_ERR_RT_ADDRESS = -6,
    BUS_ERR_SUBADDRESS = -7
} bus_status_t;

typedef struct {
    uint16_t sync;
    uint8_t rt_address;
    uint8_t subaddress;
    uint8_t word_count;
    uint8_t flags;
} bus_header_t;

typedef struct {
    bus_header_t header;
    uint16_t data[BUS_MAX_DATA_WORDS];
    uint16_t checksum;
} bus_message_t;

void bus_message_init(bus_message_t *msg, uint8_t rt_address, uint8_t subaddress);

bus_status_t bus_message_set_data(bus_message_t *msg, const uint16_t *words, uint8_t word_count);

uint16_t bus_message_compute_checksum(const bus_message_t *msg);

bus_status_t bus_message_finalize(bus_message_t *msg);

bus_status_t bus_message_serialize(const bus_message_t *msg, uint8_t *buffer, size_t buffer_size, size_t *out_len);

bus_status_t bus_message_parse(const uint8_t *buffer, size_t buffer_len, bus_message_t *out_msg);

const char *bus_status_string(bus_status_t status);

#endif /* BUS_MESSAGE_H */
