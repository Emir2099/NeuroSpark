#ifndef NET_H
#define NET_H

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

int net_init(void);
int net_is_ready(void);
int net_send_probe(void);
int net_export_snapshot(int slot);

uint32_t net_nic_io_base(void);
int net_nic_index(void);
const char *net_driver_name(void);
void net_get_mac(uint8_t out[6]);

#endif
