#ifndef REMOTE_AUTH_H
#define REMOTE_AUTH_H

#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif

#ifndef _UINT16_T_DEFINED
#define _UINT16_T_DEFINED
typedef unsigned short uint16_t;
#endif

#ifndef _UINT8_T_DEFINED
#define _UINT8_T_DEFINED
typedef unsigned char uint8_t;
#endif

#define REMOTE_PKT_MAGIC 0x52435031u
#define REMOTE_PKT_VERSION 1u
#define REMOTE_PKT_TYPE_STATUS 1
#define REMOTE_PKT_TYPE_COMMAND 2
#define REMOTE_PKT_TYPE_SNAPSHOT 3
#define REMOTE_PKT_TYPE_PROFILE 4
#define REMOTE_PKT_TYPE_MANIFEST 5

#define REMOTE_PKT_FLAG_RELIABLE 0x00000001u
#define REMOTE_PKT_FLAG_CHUNKED 0x00000002u
#define REMOTE_PKT_FLAG_CHUNK_FIRST 0x00000004u
#define REMOTE_PKT_FLAG_CHUNK_LAST 0x00000008u

#define REMOTE_CHUNK_MAGIC 0x43484B31u

#define REMOTE_STATUS_OK 0u
#define REMOTE_STATUS_ERROR 255u
#define REMOTE_STATUS_AUTH_REQUIRED 1u
#define REMOTE_STATUS_TOKEN_EXPIRED 2u
#define REMOTE_STATUS_INVALID_TOKEN 3u

#define REMOTE_ROLE_VIEWER 1u
#define REMOTE_ROLE_OPERATOR 2u
#define REMOTE_ROLE_ADMIN 3u

typedef struct {
  uint32_t tick;
  uint32_t role;
  uint32_t request_id;
  uint32_t nonce;
  uint32_t cmd_hash;
  uint32_t status;
  uint32_t rejected;
} RemoteAuditEntry;

typedef struct {
  int active;
  uint32_t transfer_id;
  uint32_t total_len;
  uint32_t received_len;
  uint32_t checksum;
  uint32_t last_chunk_index;
} RemoteTransferState;

/* Auth Management */
void remote_set_token(uint32_t token);
void remote_set_auth(uint32_t token, uint32_t ttl_ticks);
void remote_clear_auth(void);
int remote_is_authorized(void);
void remote_rotate_session_key(void);
void remote_set_role(uint32_t role);

uint32_t remote_get_session(void);
uint32_t remote_get_token(void);
uint32_t remote_get_role(void);
uint32_t remote_get_auth_deadline(void);

int remote_auth_valid(void);
uint32_t remote_next_request_id(void);
uint32_t remote_next_nonce(uint32_t request_id);
void remote_seed_nonce_state(void);
void remote_rotate_session_key_internal(void);

/* Audit Log */
void remote_audit_log(uint32_t tick, uint32_t role, uint32_t request_id, uint32_t nonce, uint32_t cmd_hash, uint32_t status, uint32_t rejected);
int remote_audit_get(RemoteAuditEntry *out, int max_count);
int remote_audit_count(void);

/* Transfer State Tracking */
void remote_transfer_begin(uint32_t transfer_id, uint32_t total_len, uint32_t checksum);
void remote_transfer_progress(uint32_t transfer_id, uint32_t chunk_index, uint32_t chunk_len);
int remote_transfer_can_resume(uint32_t transfer_id);
uint32_t remote_transfer_resume_offset(uint32_t transfer_id);
void remote_transfer_complete(uint32_t transfer_id);
int remote_transfer_get_states(RemoteTransferState *out, int max);

uint32_t remote_checksum32(const uint8_t *data, uint32_t len);
#endif
