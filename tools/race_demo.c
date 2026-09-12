#include "shm_channel.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    shm_record_t record;
    pthread_mutex_t lock;
    unsigned long torn_reads;
    unsigned long total_reads;
} race_shared_t;

static int record_is_consistent(uint32_t seq, uint32_t value_a, uint32_t value_b)
{
    return value_a == (seq * 2u) && value_b == (seq * 3u);
}

static void run_writer_broken(race_shared_t *shared, unsigned long iterations)
{
    unsigned long i = 0u;

    for (i = 1u; i <= iterations; ++i) {
        shm_record_publish_broken(&shared->record, (uint32_t)i, (uint32_t)(i * 2u), (uint32_t)(i * 3u));
    }
}

static void run_writer_fixed(race_shared_t *shared, unsigned long iterations)
{
    unsigned long i = 0u;

    for (i = 1u; i <= iterations; ++i) {
        pthread_mutex_lock(&shared->lock);
        shm_record_publish(&shared->record, (uint32_t)i, (uint32_t)(i * 2u), (uint32_t)(i * 3u));
        pthread_mutex_unlock(&shared->lock);
    }
}

static void run_reader_broken(race_shared_t *shared, unsigned long iterations)
{
    unsigned long i = 0u;

    for (i = 0u; i < iterations; ++i) {
        uint32_t seq = 0u;
        uint32_t value_a = 0u;
        uint32_t value_b = 0u;

        if (shm_record_consume_broken(&shared->record, &seq, &value_a, &value_b) != 0) {
            shared->torn_reads++;
        } else if (!record_is_consistent(seq, value_a, value_b)) {
            shared->torn_reads++;
        }

        shared->total_reads++;
    }
}

static void run_reader_fixed(race_shared_t *shared, unsigned long iterations)
{
    unsigned long i = 0u;

    for (i = 0u; i < iterations; ++i) {
        uint32_t seq = 0u;
        uint32_t value_a = 0u;
        uint32_t value_b = 0u;

        pthread_mutex_lock(&shared->lock);
        seq = shared->record.seq;
        value_a = shared->record.value_a;
        value_b = shared->record.value_b;
        pthread_mutex_unlock(&shared->lock);

        if (!record_is_consistent(seq, value_a, value_b)) {
            shared->torn_reads++;
        }

        shared->total_reads++;
    }
}

static int run_mode(const char *mode_name, int use_fixed, unsigned long iterations)
{
    race_shared_t *shared = NULL;
    pthread_mutexattr_t attr;
    pid_t pid = -1;
    int status = 0;

    shared = mmap(NULL, sizeof(*shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    memset(shared, 0, sizeof(*shared));
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (use_fixed) {
            run_writer_fixed(shared, iterations);
        } else {
            run_writer_broken(shared, iterations);
        }
        _exit(0);
    }

    if (use_fixed) {
        run_reader_fixed(shared, iterations);
    } else {
        run_reader_broken(shared, iterations);
    }

    waitpid(pid, &status, 0);

    printf("[%s] torn_reads=%lu total_reads=%lu (%.2f%%)\n",
           mode_name,
           shared->torn_reads,
           shared->total_reads,
           (100.0 * (double)shared->torn_reads) / (double)shared->total_reads);

    pthread_mutex_destroy(&shared->lock);
    munmap(shared, sizeof(*shared));
    return 0;
}

int main(int argc, char **argv)
{
    unsigned long iterations = 200000u;
    const char *mode = "both";

    if (argc > 1) {
        iterations = (unsigned long)strtoul(argv[1], NULL, 10);
    }
    if (argc > 2) {
        mode = argv[2];
    }

    if (strcmp(mode, "broken") == 0) {
        return run_mode("broken", 0, iterations);
    }
    if (strcmp(mode, "fixed") == 0) {
        return run_mode("fixed", 1, iterations);
    }

    if (run_mode("broken", 0, iterations) != 0) {
        return 1;
    }
    return run_mode("fixed", 1, iterations);
}
