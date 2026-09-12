#include "message.h"

#include <string.h>

static bus_status_t validate_header_fields(const bus_header_t *header)
{
    if (header->rt_address > 31u) {
        return BUS_ERR_RT_ADDRESS;
    }
    if (header->subaddress > 31u) {
        return BUS_ERR_SUBADDRESS;
    }
    if (header->word_count > BUS_MAX_DATA_WORDS) {
        return BUS_ERR_WORD_COUNT;
    }
    return BUS_OK;
}

void bus_message_init(bus_message_t *msg, uint8_t rt_address, uint8_t subaddress)
{
    if (msg == NULL) {
        return;
    }

    memset(msg, 0, sizeof(*msg));
    msg->header.sync = BUS_SYNC_WORD;
    msg->header.rt_address = rt_address;
    msg->header.subaddress = subaddress;
    msg->header.word_count = 0u;
    msg->header.flags = 0u;
}

bus_status_t bus_message_set_data(bus_message_t *msg, const uint16_t *words, uint8_t word_count)
{
    if (msg == NULL || words == NULL) {
        return BUS_ERR_NULL;
    }
    if (word_count > BUS_MAX_DATA_WORDS) {
        return BUS_ERR_WORD_COUNT;
    }

    memcpy(msg->data, words, (size_t)word_count * sizeof(uint16_t));
    msg->header.word_count = word_count;
    return BUS_OK;
}

uint16_t bus_message_compute_checksum(const bus_message_t *msg)
{
    uint32_t sum = 0u;

    if (msg == NULL) {
        return 0u;
    }

    sum += msg->header.sync;
    sum += msg->header.rt_address;
    sum += msg->header.subaddress;
    sum += msg->header.word_count;
    sum += msg->header.flags;

    for (uint8_t i = 0u; i < msg->header.word_count; ++i) {
        sum += msg->data[i];
    }

    return (uint16_t)(sum & 0xFFFFu);
}

bus_status_t bus_message_finalize(bus_message_t *msg)
{
    bus_status_t status;

    if (msg == NULL) {
        return BUS_ERR_NULL;
    }

    status = validate_header_fields(&msg->header);
    if (status != BUS_OK) {
        return status;
    }

    msg->checksum = bus_message_compute_checksum(msg);
    return BUS_OK;
}

static size_t message_payload_size(uint8_t word_count)
{
    return BUS_HEADER_SIZE + ((size_t)word_count * 2u) + BUS_CHECKSUM_SIZE;
}

bus_status_t bus_message_serialize(const bus_message_t *msg, uint8_t *buffer, size_t buffer_size, size_t *out_len)
{
    size_t required;
    size_t offset = 0u;
    bus_status_t status;

    if (msg == NULL || buffer == NULL || out_len == NULL) {
        return BUS_ERR_NULL;
    }

    status = validate_header_fields(&msg->header);
    if (status != BUS_OK) {
        return status;
    }

    required = message_payload_size(msg->header.word_count);
    if (buffer_size < required) {
        return BUS_ERR_SIZE;
    }

    buffer[offset++] = (uint8_t)(msg->header.sync >> 8);
    buffer[offset++] = (uint8_t)(msg->header.sync & 0xFFu);
    buffer[offset++] = msg->header.rt_address;
    buffer[offset++] = msg->header.subaddress;
    buffer[offset++] = msg->header.word_count;
    buffer[offset++] = msg->header.flags;

    for (uint8_t i = 0u; i < msg->header.word_count; ++i) {
        buffer[offset++] = (uint8_t)(msg->data[i] >> 8);
        buffer[offset++] = (uint8_t)(msg->data[i] & 0xFFu);
    }

    buffer[offset++] = (uint8_t)(msg->checksum >> 8);
    buffer[offset++] = (uint8_t)(msg->checksum & 0xFFu);

    *out_len = offset;
    return BUS_OK;
}

bus_status_t bus_message_parse(const uint8_t *buffer, size_t buffer_len, bus_message_t *out_msg)
{
    size_t required;
    size_t offset = 0u;
    uint16_t checksum = 0u;
    bus_status_t status;

    if (buffer == NULL || out_msg == NULL) {
        return BUS_ERR_NULL;
    }

    if (buffer_len < BUS_HEADER_SIZE + BUS_CHECKSUM_SIZE) {
        return BUS_ERR_SIZE;
    }

    memset(out_msg, 0, sizeof(*out_msg));

    out_msg->header.sync = (uint16_t)(((uint16_t)buffer[offset] << 8) | buffer[offset + 1u]);
    offset += 2u;

    if (out_msg->header.sync != BUS_SYNC_WORD) {
        return BUS_ERR_SYNC;
    }

    out_msg->header.rt_address = buffer[offset++];
    out_msg->header.subaddress = buffer[offset++];
    out_msg->header.word_count = buffer[offset++];
    out_msg->header.flags = buffer[offset++];

    status = validate_header_fields(&out_msg->header);
    if (status != BUS_OK) {
        return status;
    }

    required = message_payload_size(out_msg->header.word_count);
    if (buffer_len < required) {
        return BUS_ERR_SIZE;
    }

    for (uint8_t i = 0u; i < out_msg->header.word_count; ++i) {
        out_msg->data[i] = (uint16_t)(((uint16_t)buffer[offset] << 8) | buffer[offset + 1u]);
        offset += 2u;
    }

    checksum = (uint16_t)(((uint16_t)buffer[offset] << 8) | buffer[offset + 1u]);
    out_msg->checksum = checksum;

    if (checksum != bus_message_compute_checksum(out_msg)) {
        return BUS_ERR_CHECKSUM;
    }

    return BUS_OK;
}

const char *bus_status_string(bus_status_t status)
{
    switch (status) {
    case BUS_OK:
        return "ok";
    case BUS_ERR_NULL:
        return "null argument";
    case BUS_ERR_SIZE:
        return "buffer too small";
    case BUS_ERR_SYNC:
        return "invalid sync word";
    case BUS_ERR_WORD_COUNT:
        return "invalid word count";
    case BUS_ERR_CHECKSUM:
        return "checksum mismatch";
    case BUS_ERR_RT_ADDRESS:
        return "invalid rt address";
    case BUS_ERR_SUBADDRESS:
        return "invalid subaddress";
    default:
        return "unknown error";
    }
}
