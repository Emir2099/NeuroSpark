#ifndef NET_H
#define NET_H

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef struct {
	uint32_t total_frames;
	uint32_t total_bytes;
	uint32_t ipv4_frames;
	uint32_t arp_frames;
	uint32_t neuro_frames;
	uint32_t unknown_frames;
	uint32_t dropped_frames;
	uint32_t malformed_frames;
	uint32_t quarantined_frames;
	uint32_t injected_loss_frames;
	uint32_t injected_reorder_frames;
	uint32_t tcp_retransmits;
	uint32_t tcp_retransmit_failures;
	uint32_t tcp_half_open_cleanups;
	uint32_t tcp_reset_events;
	uint32_t irq_count;
	uint32_t irq_rx_events;
	uint32_t tx_probe_ok;
	uint32_t tx_probe_fail;
	uint32_t poll_budget;
	uint32_t irq_poll_budget;
	uint32_t coalesced_batches;
} NetRxStats;

int net_init(void);
int net_up(void);
int net_is_ready(void);
int net_rx_poll(void);
int net_irq_handler(void);
void net_get_rx_stats(NetRxStats *out);
void net_set_rx_coalesce(uint32_t poll_budget, uint32_t irq_budget);
void net_get_rx_coalesce(uint32_t *poll_budget, uint32_t *irq_budget);
int net_send_probe(void);
int net_export_snapshot(int slot);
int net_export_profile(void);
int net_export_manifest(void);

void net_set_fault_injection(uint32_t loss_percent, uint32_t reorder_percent);
void net_get_fault_injection(uint32_t *loss_percent, uint32_t *reorder_percent);

int net_set_ipv4(uint32_t ip, uint32_t mask, uint32_t gw);
void net_get_ipv4(uint32_t *ip, uint32_t *mask, uint32_t *gw);

int net_udp_bind(uint16_t port);
int net_udp_unbind(uint16_t port);
int net_udp_has_data(uint16_t port);
int net_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
				 const void *payload, uint16_t payload_len);
int net_udp_recv(uint16_t port, uint32_t *src_ip, uint16_t *src_port,
				 void *buf, uint16_t max_len);

int net_tcp_listen(uint16_t port);
int net_tcp_unlisten(uint16_t port);
int net_tcp_accept_ready(uint16_t listen_port);
int net_tcp_connect(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port);
int net_tcp_accept(uint16_t listen_port, uint32_t *peer_ip, uint16_t *peer_port);
int net_tcp_is_established(int conn_id);
int net_tcp_can_recv(int conn_id);
int net_tcp_can_send(int conn_id);
int net_tcp_send(int conn_id, const void *payload, uint16_t payload_len);
int net_tcp_recv(int conn_id, void *buf, uint16_t max_len);
int net_tcp_close(int conn_id);
int net_tcp_poll(void);

uint32_t net_nic_io_base(void);
int net_nic_index(void);
const char *net_driver_name(void);
void net_get_mac(uint8_t out[6]);
int net_set_mac(const uint8_t mac[6]);
int net_link_speed_mbps(void);
uint16_t net_mtu_bytes(void);
int net_supports_jumbo(void);

void remote_set_enabled(int enabled);
int remote_is_enabled(void);
int remote_send_command_result(const char *cmd_text);

#endif
