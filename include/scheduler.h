#ifndef BUS_SCHEDULER_H
#define BUS_SCHEDULER_H

#include "message.h"

#include <stddef.h>

#define BUS_MAX_RT_NODES 8u

typedef struct {
    uint8_t rt_addresses[BUS_MAX_RT_NODES];
    size_t node_count;
    size_t current_index;
    uint32_t grant_budget;
    uint32_t grants_left;
} bus_scheduler_t;

void bus_scheduler_init(bus_scheduler_t *sched, uint32_t grant_budget);

bus_status_t bus_scheduler_register(bus_scheduler_t *sched, uint8_t rt_address);

bus_status_t bus_scheduler_next(bus_scheduler_t *sched, uint8_t *rt_address);

void bus_scheduler_reset_cycle(bus_scheduler_t *sched);

#endif /* BUS_SCHEDULER_H */
