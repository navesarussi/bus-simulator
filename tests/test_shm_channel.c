#include "shm_channel.h"

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

static void test_atomic_record_round_trip(void)
{
    shm_record_t record;
    uint32_t value_a = 0u;
    uint32_t value_b = 0u;

    memset(&record, 0, sizeof(record));
    shm_record_publish(&record, 42u, 0x11111111u, 0x22222222u);
    expect_true(shm_record_consume(&record, 42u, &value_a, &value_b) == 0, "atomic consume succeeds");
    expect_true(value_a == 0x11111111u, "atomic value_a preserved");
    expect_true(value_b == 0x22222222u, "atomic value_b preserved");
}

static void test_ring_buffer_publish_consume(void)
{
    shm_channel_t channel;
    shm_record_t in;
    shm_record_t out;

    memset(&channel, 0, sizeof(channel));
    memset(&in, 0, sizeof(in));

    shm_record_publish(&in, 1u, 10u, 20u);
    expect_true(shm_channel_publish(&channel, &in) == BUS_OK, "publish to ring");
    expect_true(shm_channel_try_consume(&channel, &out) == BUS_OK, "consume from ring");
    expect_true(out.seq == 1u && out.value_a == 10u && out.value_b == 20u, "ring payload preserved");
}

int main(void)
{
    test_atomic_record_round_trip();
    test_ring_buffer_publish_consume();

    if (failures != 0) {
        fprintf(stderr, "%d shm test(s) failed\n", failures);
        return 1;
    }

    printf("All shm tests passed.\n");
    return 0;
}
