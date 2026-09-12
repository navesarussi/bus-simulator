#include "shm_channel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_seconds(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ((double)ts.tv_nsec / 1e9);
}

int main(int argc, char **argv)
{
    shm_channel_t channel;
    shm_record_t record;
    shm_record_t out;
    unsigned long iterations = 500000u;
    unsigned long i = 0u;
    double start = 0.0;
    double end = 0.0;

    if (argc > 1) {
        iterations = (unsigned long)strtoul(argv[1], NULL, 10);
    }

    memset(&channel, 0, sizeof(channel));
    memset(&record, 0, sizeof(record));

    start = now_seconds();
    for (i = 0u; i < iterations; ++i) {
        shm_record_publish(&record, (uint32_t)i + 1u, (uint32_t)i, (uint32_t)(i + 17u));
        if (shm_channel_publish(&channel, &record) != BUS_OK) {
            fprintf(stderr, "publish failed at iteration %lu\n", i);
            return 1;
        }
        if (shm_channel_try_consume(&channel, &out) != BUS_OK) {
            fprintf(stderr, "consume failed at iteration %lu\n", i);
            return 1;
        }
    }
    end = now_seconds();

    printf("iterations=%lu elapsed=%.6f seconds throughput=%.0f msg/s\n",
           iterations,
           end - start,
           (double)iterations / (end - start));
    return 0;
}
