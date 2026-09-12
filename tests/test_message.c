#include "message.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void expect_true(int condition, const char *label)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        failures++;
    } else {
        printf("PASS: %s\n", label);
    }
}

static void test_round_trip(void)
{
    bus_message_t original;
    bus_message_t parsed;
    uint8_t buffer[BUS_MAX_MESSAGE_SIZE];
    size_t serialized_len = 0u;
    const uint16_t payload[] = {0x1234u, 0xABCDu, 0x00FFu};
    bus_status_t status;

    bus_message_init(&original, 5u, 2u);
    status = bus_message_set_data(&original, payload, 3u);
    expect_true(status == BUS_OK, "set_data returns ok");

    status = bus_message_finalize(&original);
    expect_true(status == BUS_OK, "finalize returns ok");

    status = bus_message_serialize(&original, buffer, sizeof(buffer), &serialized_len);
    expect_true(status == BUS_OK, "serialize returns ok");
    expect_true(serialized_len == 14u, "serialized length is 14 bytes");

    status = bus_message_parse(buffer, serialized_len, &parsed);
    expect_true(status == BUS_OK, "parse returns ok");
    expect_true(parsed.header.rt_address == 5u, "rt address preserved");
    expect_true(parsed.header.subaddress == 2u, "subaddress preserved");
    expect_true(parsed.header.word_count == 3u, "word count preserved");
    expect_true(parsed.data[0] == 0x1234u, "data word 0 preserved");
    expect_true(parsed.data[1] == 0xABCDu, "data word 1 preserved");
    expect_true(parsed.data[2] == 0x00FFu, "data word 2 preserved");
    expect_true(parsed.checksum == original.checksum, "checksum preserved");
}

static void test_rejects_bad_checksum(void)
{
    bus_message_t original;
    bus_message_t parsed;
    uint8_t buffer[BUS_MAX_MESSAGE_SIZE];
    size_t serialized_len = 0u;
    bus_status_t status;

    bus_message_init(&original, 1u, 1u);
    status = bus_message_finalize(&original);
    expect_true(status == BUS_OK, "finalize baseline message");

    status = bus_message_serialize(&original, buffer, sizeof(buffer), &serialized_len);
    expect_true(status == BUS_OK, "serialize baseline message");

    buffer[serialized_len - 1u] ^= 0xFFu;

    status = bus_message_parse(buffer, serialized_len, &parsed);
    expect_true(status == BUS_ERR_CHECKSUM, "parse rejects corrupted checksum");
}

int main(void)
{
    test_round_trip();
    test_rejects_bad_checksum();

    if (failures != 0) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }

    printf("All message tests passed.\n");
    return 0;
}
