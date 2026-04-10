#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

REPORT_FILE="phase27_validation.txt"
KERNEL_STRINGS_FILE=".phase27_strings.txt"
PASS_COUNT=0
FAIL_COUNT=0

log() {
  printf '%s\n' "$*"
}

pass() {
  PASS_COUNT=$((PASS_COUNT + 1))
  log "[PASS] $*"
}

fail() {
  FAIL_COUNT=$((FAIL_COUNT + 1))
  log "[FAIL] $*"
}

assert_grep() {
  local name="$1"
  local pattern="$2"
  local file="$3"
  if grep -E -q "$pattern" "$file"; then
    pass "$name"
  else
    fail "$name"
  fi
}

assert_string_in_kernel() {
  local name="$1"
  local needle="$2"
  if grep -F -q "$needle" "$KERNEL_STRINGS_FILE"; then
    pass "$name"
  else
    fail "$name"
  fi
}

log "== Phase 27 Validation (Automated Portion) =="

log "[1/4] Build image"
make NeuroSpark.bin >/dev/null
pass "Build completed"

log "[2/4] Source checks"
assert_grep "Effective priority path present" "scheduler_effective_priority_locked|SCHED_WAIT_BOOST_MAX" "kernel/scheduler.c"
assert_grep "Wake boost accounting present" "io_wake_boosts|sched_wake_boost" "kernel/scheduler.c"
assert_grep "Starvation mitigation metric present" "starvation_mitigations|SCHED_STARVATION_WAIT_TICKS" "kernel/scheduler.c"
assert_grep "Inversion hint metric present" "inversion_hints" "kernel/scheduler.c"
assert_grep "Scheduler metrics API present" "scheduler_get_metrics|scheduler_reset_metrics" "kernel/scheduler.c"
assert_grep "Task scheduler tuning fields present" "sched_wait_ticks|sched_wake_boost|sched_last_run_tick" "kernel/task.h"
assert_grep "PMM stats and slab APIs present" "pmm_get_stats|slab_alloc|slab_get_stats" "kernel/pmm.c"
assert_grep "Disk elevator batch path present" "disk_process_batch|DiskSchedulerStats" "kernel/disk.c"
assert_grep "AIO syscalls wired" "SYS_IO_SETUP|SYS_IO_SUBMIT|SYS_IO_GETEVENTS|aio_process_events" "kernel/syscall.c"
assert_grep "Net coalescing controls present" "net_set_rx_coalesce|coalesced_batches" "kernel/net.c"

log "[3/4] Shell command surface checks"
strings kernel.bin > "$KERNEL_STRINGS_FILE"
assert_string_in_kernel "sched command compiled" "SCHED SHOW"
assert_string_in_kernel "sched reset compiled" "SCHED RESET"
assert_string_in_kernel "phase27 help text compiled" "phase27:  sched show|reset"
assert_string_in_kernel "nice range updated" "USAGE: nice <pid> <0..7>"
assert_string_in_kernel "memstat command compiled" "MEMSTAT OK"
assert_string_in_kernel "net coalesce command compiled" "NET COALESCE OK"
rm -f "$KERNEL_STRINGS_FILE"

log "[4/4] Write report"
{
  echo "Phase 27 Validation Report"
  echo "Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
  echo "Automated pass count: $PASS_COUNT"
  echo "Automated fail count: $FAIL_COUNT"
  if [[ $FAIL_COUNT -eq 0 ]]; then
    echo "Automated result: PASS"
  else
    echo "Automated result: FAIL"
  fi
  echo
  echo
  echo "Suggested shell sequence for manual validation:"
  echo "  sched reset"
  echo "  ps"
  echo "  nice 1 0"
  echo "  nice 2 7"
  echo "  sched show"
  echo "  memstat"
  echo "  net coalesce 48 160"
  echo "  net status"
} >"$REPORT_FILE"

if [[ $FAIL_COUNT -eq 0 ]]; then
  log "== Phase 27 Automated Validation PASS =="
  log "Report: $REPORT_FILE"
  exit 0
fi

log "== Phase 27 Automated Validation FAIL =="
log "Report: $REPORT_FILE"
exit 1
