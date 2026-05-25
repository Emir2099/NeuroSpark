#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

REPORT_FILE="phase26_validation.txt"
KERNEL_STRINGS_FILE=".phase26_strings.txt"
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

log "== Phase 26 Validation (Automated Portion) =="

log "[1/4] Build image"
make NeuroSpark.bin >/dev/null
pass "Build completed"

log "[2/4] Source-level checks"
assert_grep "POSIX identity core present" "posix_get_uid|posix_set_uid|posix_set_gid" "kernel/posix.c"
assert_grep "User/group lookup present" "posix_lookup_user_name|posix_lookup_group_name" "kernel/posix.c"
assert_grep "Path permission checks present" "posix_check_path_permission" "kernel/posix.c"
assert_grep "Setuid/setgid exec hook present" "posix_apply_exec_credentials" "kernel/usermode.c"
assert_grep "TTY fg group control present" "posix_tty_tcsetpgrp|posix_tty_tcgetpgrp" "kernel/posix.c"
assert_grep "Signal to process group present" "posix_task_signal_pgid" "kernel/posix.c"
assert_grep "SIGTSTP/SIGCONT semantics in syscall" "SIGTSTP|SIGCONT|TASK_STATE_BLOCKED" "kernel/syscall.c"
assert_grep "Foreground signals from input" "posix_tty_signal_foreground" "kernel/input.c"

log "[3/4] Shell command surface checks"
strings kernel.bin > "$KERNEL_STRINGS_FILE"
assert_string_in_kernel "id command compiled" "ID OK"
assert_string_in_kernel "chmod command compiled" "CHMOD OK"
assert_string_in_kernel "chown command compiled" "CHOWN OK"
assert_string_in_kernel "setuid command compiled" "SETUID OK"
assert_string_in_kernel "setgid command compiled" "SETGID OK"
assert_string_in_kernel "jobs command compiled" "JOBS OK"
assert_string_in_kernel "fg/bg commands compiled" "FG OK"
assert_string_in_kernel "fg/bg commands compiled (bg)" "BG OK"
assert_string_in_kernel "tcsetpgrp command compiled" "TCSETPGRP OK"
assert_string_in_kernel "tcgetpgrp command compiled" "TCGETPGRP OK"
rm -f "$KERNEL_STRINGS_FILE"

log "[4/4] Write report"
{
  echo "Phase 26 Validation Report"
  echo "Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
  echo "Automated pass count: $PASS_COUNT"
  echo "Automated fail count: $FAIL_COUNT"
  if [[ $FAIL_COUNT -eq 0 ]]; then
    echo "Automated result: PASS"
  else
    echo "Automated result: FAIL"
  fi
  echo
  echo "Suggested shell sequence for manual validation:"
  echo "  id"
  echo "  chmod /demo.bin 755"
  echo "  chown /demo.bin 1000 100"
  echo "  setuid 1001"
  echo "  id"
  echo "  exec /demo.bin"
  echo "  jobs"
  echo "  setpgid 2 2"
  echo "  tcsetpgrp 0 2"
  echo "  tcgetpgrp 0"
  echo "  fg 2"
  echo "  bg 2"
} >"$REPORT_FILE"

if [[ $FAIL_COUNT -eq 0 ]]; then
  log "== Phase 26 Automated Validation PASS =="
  log "Report: $REPORT_FILE"
  exit 0
fi

log "== Phase 26 Automated Validation FAIL =="
log "Report: $REPORT_FILE"
exit 1
