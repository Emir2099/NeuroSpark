#ifndef NET_H
#define NET_H

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

int net_init(void);
int net_up(void);
int net_is_ready(void);
int net_send_probe(void);
int net_export_snapshot(int slot);
int net_export_profile(void);
int net_export_manifest(void);

int net_set_ipv4(uint32_t ip, uint32_t mask, uint32_t gw);
void net_get_ipv4(uint32_t *ip, uint32_t *mask, uint32_t *gw);

uint32_t net_nic_io_base(void);
int net_nic_index(void);
const char *net_driver_name(void);
void net_get_mac(uint8_t out[6]);

void remote_set_enabled(int enabled);
int remote_is_enabled(void);
void remote_set_token(uint32_t token);
void remote_set_auth(uint32_t token, uint32_t ttl_ticks);
void remote_clear_auth(void);
int remote_is_authorized(void);
uint32_t remote_get_token(void);
uint32_t remote_get_session(void);
uint32_t remote_get_auth_deadline(void);
int remote_send_command_result(const char *cmd_text);

#endif
