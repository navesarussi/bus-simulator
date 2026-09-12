#include "scheduler.h"

static int scheduler_contains(const bus_scheduler_t *sched, uint8_t rt_address)
{
    for (size_t i = 0u; i < sched->node_count; ++i) {
        if (sched->rt_addresses[i] == rt_address) {
            return 1;
        }
    }
    return 0;
}

void bus_scheduler_init(bus_scheduler_t *sched, uint32_t grant_budget)
{
    if (sched == NULL) {
        return;
    }

    sched->node_count = 0u;
    sched->current_index = 0u;
    sched->grant_budget = grant_budget == 0u ? 1u : grant_budget;
    sched->grants_left = 0u;
}

bus_status_t bus_scheduler_register(bus_scheduler_t *sched, uint8_t rt_address)
{
    if (sched == NULL) {
        return BUS_ERR_NULL;
    }
    if (rt_address > 31u) {
        return BUS_ERR_RT_ADDRESS;
    }
    if (scheduler_contains(sched, rt_address)) {
        return BUS_OK;
    }
    if (sched->node_count >= BUS_MAX_RT_NODES) {
        return BUS_ERR_SIZE;
    }

    sched->rt_addresses[sched->node_count++] = rt_address;
    return BUS_OK;
}

void bus_scheduler_reset_cycle(bus_scheduler_t *sched)
{
    if (sched == NULL) {
        return;
    }

    sched->current_index = 0u;
    sched->grants_left = 0u;
}

bus_status_t bus_scheduler_next(bus_scheduler_t *sched, uint8_t *rt_address)
{
    if (sched == NULL || rt_address == NULL) {
        return BUS_ERR_NULL;
    }
    if (sched->node_count == 0u) {
        return BUS_ERR_SIZE;
    }

    if (sched->grants_left > 0u) {
        *rt_address = sched->rt_addresses[(sched->current_index + sched->node_count - 1u) % sched->node_count];
        sched->grants_left--;
        return BUS_OK;
    }

    *rt_address = sched->rt_addresses[sched->current_index];
    sched->grants_left = sched->grant_budget - 1u;
    sched->current_index = (sched->current_index + 1u) % sched->node_count;
    return BUS_OK;
}
