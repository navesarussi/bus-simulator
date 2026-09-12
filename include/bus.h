#ifndef BUS_H
#define BUS_H

#include "message.h"
#include "scheduler.h"

#include <stddef.h>

typedef struct {
    bus_scheduler_t scheduler;
    bus_message_t mailboxes[BUS_MAX_RT_NODES];
    int mailbox_used[BUS_MAX_RT_NODES];
    uint8_t mailbox_rt[BUS_MAX_RT_NODES];
    size_t mailbox_count;
} bus_controller_t;

void bus_controller_init(bus_controller_t *bus, uint32_t grant_budget);

bus_status_t bus_controller_attach(bus_controller_t *bus, uint8_t rt_address);

bus_status_t bus_controller_submit(bus_controller_t *bus, const bus_message_t *msg);

bus_status_t bus_controller_dispatch(bus_controller_t *bus, bus_message_t *out_msg, uint8_t *out_rt);

#endif /* BUS_H */
