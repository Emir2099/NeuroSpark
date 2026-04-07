#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

REPORT_FILE="phase20_validation.txt"
TMP_DIR=".phase20_tmp"
mkdir -p "$TMP_DIR"

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
  if strings kernel.bin | grep -F -q "$needle"; then
    pass "$name"
  else
    fail "$name"
  fi
}

run_smoke() {
  local name="$1"
  shift
  local out_file="$TMP_DIR/${name}.log"
  local rc=0

  log "[RUN] $name"
  if timeout 20s qemu-system-i386 "$@" >"$out_file" 2>&1; then
    rc=0
  else
    rc=$?
  fi

  if [[ $rc -eq 124 ]] || grep -qi "terminating on signal 15" "$out_file"; then
    pass "$name (stable for timeout window)"
    return 0
  fi

  if [[ $rc -eq 0 ]]; then
    pass "$name (exited cleanly)"
    return 0
  fi

  fail "$name (exit=$rc)"
  tail -n 40 "$out_file" || true
  return 1
}

log "== Phase 20 Validation (Automated Portion) =="

log "[1/5] Build image"
make NeuroSpark.bin >/dev/null
pass "Build completed"

log "[2/5] Source-level implementation checks"
assert_grep "MBR parser present" "partition_scan_mbr" "kernel/partition.c"
assert_grep "GPT parser present" "partition_scan_gpt" "kernel/partition.c"
assert_grep "Ext2 backend resolver present" "ext2_backend_from_spec" "kernel/ext2fs.c"
assert_grep "Page cache stats present" "page_cache_get_stats|page_cache_reset_stats" "kernel/page_cache.c"
assert_grep "FD table size set to 256" "#define TASK_MAX_FDS 256" "kernel/task.h"
assert_grep "dup syscall path present" "SYS_DUP" "kernel/syscall.c"
assert_grep "dup2 syscall path present" "SYS_DUP2" "kernel/syscall.c"
assert_grep "fcntl syscall path present" "SYS_FCNTL" "kernel/syscall.c"
assert_grep "lseek syscall path present" "SYS_LSEEK" "kernel/syscall.c"
assert_grep "Task exit fd cleanup present" "vfs_close_all_for_task" "kernel/scheduler.c"

log "[3/5] Binary marker checks"
assert_string_in_kernel "mount command compiled" "MOUNT OK"
assert_string_in_kernel "umount command compiled" "UMOUNT OK"
assert_string_in_kernel "ext2 ls -l header compiled" "INODE SIZE MTIME NAME"
assert_string_in_kernel "pcache command compiled" "pcache"
assert_string_in_kernel "pcache stress path compiled" "PCACHE STRESS OK"
assert_string_in_kernel "ramdisk backend compiled" "ramdisk"
assert_string_in_kernel "ext2 backend compiled" "ext2"

log "[4/5] QEMU smoke (non-interactive stability)"
run_smoke "phase20_pc_qemu32" \
  -machine pc -cpu qemu32 -display none -no-reboot -no-shutdown \
  -rtc base=localtime,clock=host -drive file=NeuroSpark.bin,format=raw,index=0,media=disk || true

run_smoke "phase20_q35" \
  -machine q35 -display none -no-reboot -no-shutdown \
  -drive file=NeuroSpark.bin,format=raw,index=0,media=disk || true

log "[5/5] Write report"
{
  echo "Phase 20 Validation Report"
  echo "Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
  echo "Automated pass count: $PASS_COUNT"
  echo "Automated fail count: $FAIL_COUNT"
  if [[ $FAIL_COUNT -eq 0 ]]; then
    echo "Automated result: PASS"
  else
    echo "Automated result: FAIL"
  fi
  echo
  echo "Manual runtime checks still required for strict exit criteria:"
  echo "1) ext2 write persistence across reboot"
  echo "2) live cache hit/miss counter movement under repeated reads"
  echo "3) 1000+ sequential 4KB write coalescing measured live"
  echo "4) memory-pressure eviction observed live without crash"
  echo
  echo "Manual commands to run in NeuroSpark shell:"
  echo "  mount / ext2"
  echo "  mkdemo"
  echo "  ls -l"
  echo "  pcache reset"
  echo "  pcache stress 1000"
  echo "  pcache show"
} >"$REPORT_FILE"

rm -rf "$TMP_DIR"

if [[ $FAIL_COUNT -eq 0 ]]; then
  log "== Phase 20 Automated Validation PASS =="
  log "Report: $REPORT_FILE"
  exit 0
fi

log "== Phase 20 Automated Validation FAIL =="
log "Report: $REPORT_FILE"
exit 1
