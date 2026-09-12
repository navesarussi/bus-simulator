#include "bus.h"

#include <string.h>

static int find_mailbox(const bus_controller_t *bus, uint8_t rt_address)
{
    for (size_t i = 0u; i < bus->mailbox_count; ++i) {
        if (bus->mailbox_rt[i] == rt_address) {
            return (int)i;
        }
    }
    return -1;
}

void bus_controller_init(bus_controller_t *bus, uint32_t grant_budget)
{
    if (bus == NULL) {
        return;
    }

    memset(bus, 0, sizeof(*bus));
    bus_scheduler_init(&bus->scheduler, grant_budget);
}

bus_status_t bus_controller_attach(bus_controller_t *bus, uint8_t rt_address)
{
    bus_status_t status;

    if (bus == NULL) {
        return BUS_ERR_NULL;
    }

    status = bus_scheduler_register(&bus->scheduler, rt_address);
    if (status != BUS_OK) {
        return status;
    }

    if (find_mailbox(bus, rt_address) >= 0) {
        return BUS_OK;
    }
    if (bus->mailbox_count >= BUS_MAX_RT_NODES) {
        return BUS_ERR_SIZE;
    }

    bus->mailbox_rt[bus->mailbox_count] = rt_address;
    bus->mailbox_used[bus->mailbox_count] = 0;
    bus->mailbox_count++;
    return BUS_OK;
}

bus_status_t bus_controller_submit(bus_controller_t *bus, const bus_message_t *msg)
{
    int slot;

    if (bus == NULL || msg == NULL) {
        return BUS_ERR_NULL;
    }

    slot = find_mailbox(bus, msg->header.rt_address);
    if (slot < 0) {
        return BUS_ERR_RT_ADDRESS;
    }

    bus->mailboxes[slot] = *msg;
    bus->mailbox_used[slot] = 1;
    return BUS_OK;
}

bus_status_t bus_controller_dispatch(bus_controller_t *bus, bus_message_t *out_msg, uint8_t *out_rt)
{
    bus_status_t status;
    uint8_t rt_address = 0u;
    size_t attempts = 0u;

    if (bus == NULL || out_msg == NULL || out_rt == NULL) {
        return BUS_ERR_NULL;
    }

    while (attempts < bus->mailbox_count) {
        int slot;

        status = bus_scheduler_next(&bus->scheduler, &rt_address);
        if (status != BUS_OK) {
            return status;
        }

        slot = find_mailbox(bus, rt_address);
        if (slot >= 0 && bus->mailbox_used[slot]) {
            *out_msg = bus->mailboxes[slot];
            *out_rt = rt_address;
            bus->mailbox_used[slot] = 0;
            return BUS_OK;
        }

        attempts++;
    }

    return BUS_ERR_SIZE;
}
