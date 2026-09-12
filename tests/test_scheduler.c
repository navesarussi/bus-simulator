#include "bus.h"
#include "scheduler.h"

#include <stdio.h>

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

static void test_round_robin_order(void)
{
    bus_scheduler_t sched;
    uint8_t rt = 0u;

    bus_scheduler_init(&sched, 1u);
    expect_true(bus_scheduler_register(&sched, 2u) == BUS_OK, "register rt 2");
    expect_true(bus_scheduler_register(&sched, 5u) == BUS_OK, "register rt 5");
    expect_true(bus_scheduler_register(&sched, 7u) == BUS_OK, "register rt 7");

    expect_true(bus_scheduler_next(&sched, &rt) == BUS_OK && rt == 2u, "first grant rt 2");
    expect_true(bus_scheduler_next(&sched, &rt) == BUS_OK && rt == 5u, "second grant rt 5");
    expect_true(bus_scheduler_next(&sched, &rt) == BUS_OK && rt == 7u, "third grant rt 7");
    expect_true(bus_scheduler_next(&sched, &rt) == BUS_OK && rt == 2u, "wrap to rt 2");
}

static void test_bus_dispatch_respects_scheduler(void)
{
    bus_controller_t bus;
    bus_message_t msg_a;
    bus_message_t msg_b;
    bus_message_t out;
    uint8_t out_rt = 0u;

    bus_controller_init(&bus, 1u);
    expect_true(bus_controller_attach(&bus, 1u) == BUS_OK, "attach rt 1");
    expect_true(bus_controller_attach(&bus, 3u) == BUS_OK, "attach rt 3");

    bus_message_init(&msg_a, 3u, 1u);
    bus_message_init(&msg_b, 1u, 2u);
    expect_true(bus_controller_submit(&bus, &msg_a) == BUS_OK, "submit rt 3");
    expect_true(bus_controller_submit(&bus, &msg_b) == BUS_OK, "submit rt 1");

    expect_true(bus_controller_dispatch(&bus, &out, &out_rt) == BUS_OK && out_rt == 1u, "dispatch rt 1 first");
    expect_true(bus_controller_dispatch(&bus, &out, &out_rt) == BUS_OK && out_rt == 3u, "dispatch rt 3 second");
}

int main(void)
{
    test_round_robin_order();
    test_bus_dispatch_respects_scheduler();

    if (failures != 0) {
        fprintf(stderr, "%d scheduler test(s) failed\n", failures);
        return 1;
    }

    printf("All scheduler tests passed.\n");
    return 0;
}
