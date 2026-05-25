#include "remote_auth.h"
extern uint32_t tick;

extern uint32_t tick;

/* Auth State */
static uint32_t remote_token = 0;
static uint32_t remote_session = 0;
static uint32_t remote_auth_deadline = 0;
static uint32_t remote_key_epoch = 0;
static uint32_t remote_nonce_state = 0;
static uint32_t remote_seq = 1;
static uint32_t remote_role = REMOTE_ROLE_VIEWER;

uint32_t remote_checksum32(const uint8_t *data, uint32_t len) {
  uint32_t hash = 2166136261u;
  for (uint32_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}


/* Audit Ring Buffer */
#define AUDIT_RING_SIZE 32
static RemoteAuditEntry audit_ring[AUDIT_RING_SIZE];
static int audit_head = 0;
static int audit_count = 0;

/* Transfer States */
#define MAX_TRANSFERS 4
static RemoteTransferState transfer_states[MAX_TRANSFERS];

/* Auth Management Implementations */

void remote_seed_nonce_state(void) {
  remote_nonce_state = remote_token ^ remote_session ^ remote_key_epoch ^ 0x9E3779B9u;
  if (remote_nonce_state == 0u) {
    remote_nonce_state = 0xA341316Cu;
  }
}

uint32_t remote_next_request_id(void) {
  uint32_t req = remote_seq++;
  if (req == 0) {
    req = remote_seq++;
  }
  return req;
}

uint32_t remote_next_nonce(uint32_t request_id) {
  remote_nonce_state ^= remote_nonce_state << 13;
  remote_nonce_state ^= remote_nonce_state >> 17;
  remote_nonce_state ^= remote_nonce_state << 5;
  remote_nonce_state ^= request_id;
  if (remote_nonce_state == 0u) {
    remote_nonce_state = 0x7F4A7C15u ^ request_id;
  }
  return remote_nonce_state;
}

void remote_rotate_session_key_internal(void) {
  uint32_t base = (tick ^ remote_token ^ (remote_key_epoch * 0x45D9F3Bu));
  remote_key_epoch++;
  remote_session = (base | 1u);
  remote_seq = 1u;
  remote_seed_nonce_state();
}

uint32_t remote_get_session(void) {
  if (remote_session == 0) {
    remote_rotate_session_key_internal();
  }
  return remote_session;
}

uint32_t remote_get_token(void) {
  return remote_token;
}

uint32_t remote_get_role(void) {
  return remote_role;
}

uint32_t remote_get_auth_deadline(void) {
  return remote_auth_deadline;
}

int remote_auth_valid(void) {
  if (remote_token == 0) {
    return 0;
  }
  if (remote_auth_deadline == 0) {
    return 0;
  }
  if (tick > remote_auth_deadline) {
    return 0;
  }
  if (remote_role < REMOTE_ROLE_VIEWER || remote_role > REMOTE_ROLE_ADMIN) {
    return 0;
  }
  return 1;
}

/* Audit Log Implementations */

void remote_audit_log(uint32_t t, uint32_t role, uint32_t request_id, uint32_t nonce, uint32_t cmd_hash, uint32_t status, uint32_t rejected) {
  RemoteAuditEntry *entry = &audit_ring[audit_head];
  entry->tick = t;
  entry->role = role;
  entry->request_id = request_id;
  entry->nonce = nonce;
  entry->cmd_hash = cmd_hash;
  entry->status = status;
  entry->rejected = rejected;
  
  audit_head = (audit_head + 1) % AUDIT_RING_SIZE;
  if (audit_count < AUDIT_RING_SIZE) {
    audit_count++;
  }
}

int remote_audit_get(RemoteAuditEntry *out, int max_count) {
  int count = 0;
  if (out == 0 || max_count <= 0) {
    return 0;
  }
  int current = audit_head - 1;
  if (current < 0) {
    current = AUDIT_RING_SIZE - 1;
  }
  while (count < max_count && count < audit_count) {
    out[count] = audit_ring[current];
    count++;
    current--;
    if (current < 0) {
      current = AUDIT_RING_SIZE - 1;
    }
  }
  return count;
}

int remote_audit_count(void) {
  return audit_count;
}

/* Transfer State Implementations */

void remote_transfer_begin(uint32_t transfer_id, uint32_t total_len, uint32_t checksum) {
  int slot = -1;
  for (int i = 0; i < MAX_TRANSFERS; i++) {
    if (!transfer_states[i].active) {
      slot = i;
      break;
    }
  }
  if (slot == -1) {
    slot = 0; // Evict oldest/first
  }
  transfer_states[slot].active = 1;
  transfer_states[slot].transfer_id = transfer_id;
  transfer_states[slot].total_len = total_len;
  transfer_states[slot].received_len = 0;
  transfer_states[slot].checksum = checksum;
  transfer_states[slot].last_chunk_index = 0;
}

void remote_transfer_progress(uint32_t transfer_id, uint32_t chunk_index, uint32_t chunk_len) {
  for (int i = 0; i < MAX_TRANSFERS; i++) {
    if (transfer_states[i].active && transfer_states[i].transfer_id == transfer_id) {
      transfer_states[i].received_len += chunk_len;
      transfer_states[i].last_chunk_index = chunk_index;
      return;
    }
  }
}

int remote_transfer_can_resume(uint32_t transfer_id) {
  for (int i = 0; i < MAX_TRANSFERS; i++) {
    if (transfer_states[i].active && transfer_states[i].transfer_id == transfer_id) {
      return transfer_states[i].received_len < transfer_states[i].total_len;
    }
  }
  return 0;
}

uint32_t remote_transfer_resume_offset(uint32_t transfer_id) {
  for (int i = 0; i < MAX_TRANSFERS; i++) {
    if (transfer_states[i].active && transfer_states[i].transfer_id == transfer_id) {
      return transfer_states[i].received_len;
    }
  }
  return 0;
}

void remote_transfer_complete(uint32_t transfer_id) {
  for (int i = 0; i < MAX_TRANSFERS; i++) {
    if (transfer_states[i].active && transfer_states[i].transfer_id == transfer_id) {
      transfer_states[i].active = 0;
      return;
    }
  }
}

int remote_transfer_get_states(RemoteTransferState *out, int max) {
  int count = 0;
  if (out == 0 || max <= 0) return 0;
  for (int i = 0; i < MAX_TRANSFERS && count < max; i++) {
    if (transfer_states[i].active) {
      out[count++] = transfer_states[i];
    }
  }
  return count;
}


void remote_set_token(uint32_t token) {
  remote_token = token;
  remote_seed_nonce_state();
}

void remote_set_auth(uint32_t token, uint32_t ttl_ticks) {
  remote_token = token;
  remote_auth_deadline = ttl_ticks ? (tick + ttl_ticks) : 0;
  remote_rotate_session_key_internal();
}

void remote_clear_auth(void) {
  remote_auth_deadline = 0;
  remote_token = 0;
}

int remote_is_authorized(void) { return remote_auth_valid(); }
void remote_rotate_session_key(void) { remote_rotate_session_key_internal(); }

void remote_set_role(uint32_t role) {
  if (role < REMOTE_ROLE_VIEWER) {
    role = REMOTE_ROLE_VIEWER;
  }
  if (role > REMOTE_ROLE_ADMIN) {
    role = REMOTE_ROLE_ADMIN;
  }
  remote_role = role;
}
