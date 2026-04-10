#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

REPORT_FILE="phase28_validation.txt"
KERNEL_STRINGS_FILE=".phase28_strings.txt"
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

log "== Phase 28 Validation (Automated Portion) =="

log "[1/4] Build image"
make NeuroSpark.bin >/dev/null
pass "Build completed"

log "[2/4] Source checks"
assert_grep "Trace syscall wiring present" "SYS_TRACE|SYS_PTRACE" "kernel/syscall.c"
assert_grep "Ptrace info/stack records present" "NsPtraceInfo|NsPtraceStack|ptrace_fill_stack" "kernel/syscall.c"
assert_grep "Syscall arg logging present" "task_trace_syscall_ex|trace_last_syscall|trace_last_result" "kernel/task.c"
assert_grep "Profiler sampling hooks present" "profile_sample_current_task|profile_get_stack_samples|PROFILE_SAMPLE_MAX" "kernel/profiling.c"
assert_grep "Timer sampling hook present" "profile_sample_current_task\(os_current_task" "kernel/kernel.c"
assert_grep "Debugger shell command present" "cmd_kdbg|KDBG PID|kdbg stack" "kernel/shell.c"

log "[3/4] Shell command surface checks"
strings kernel.bin > "$KERNEL_STRINGS_FILE"
assert_string_in_kernel "phase28 help text compiled" "phase28:  kdbg show|stack|mem|trace"
assert_string_in_kernel "kdbg command compiled" "KDBG PID"
assert_string_in_kernel "trace syscall fields compiled" "SYSCALL:"
assert_string_in_kernel "profile samples command compiled" "PROFILE SAMPLES"
assert_string_in_kernel "profile export format compiled" "PRF3"
assert_string_in_kernel "profile export samples compiled" "stack="
rm -f "$KERNEL_STRINGS_FILE"

log "[4/4] Write report"
{
  echo "Phase 28 Validation Report"
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
  echo "  profile on"
  echo "  profile samples 4"
  echo "  trace 1 show"
  echo "  kdbg show 1"
  echo "  kdbg stack 1"
  echo "  kdbg mem 0xC0000000 64"
} >"$REPORT_FILE"

if [[ $FAIL_COUNT -eq 0 ]]; then
  log "== Phase 28 Automated Validation PASS =="
  log "Report: $REPORT_FILE"
  exit 0
fi

log "== Phase 28 Automated Validation FAIL =="
log "Report: $REPORT_FILE"
exit 1