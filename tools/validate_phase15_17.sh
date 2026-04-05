#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

REPORT_FILE="phase15_17_validation.txt"
TMP_DIR=".phase15_17_tmp"
mkdir -p "$TMP_DIR"

PASS_COUNT=0
FAIL_COUNT=0

log() {
  printf '%s\n' "$*"
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
    log "[PASS] $name (stable for timeout window)"
    PASS_COUNT=$((PASS_COUNT + 1))
    return 0
  fi

  if [[ $rc -eq 0 ]]; then
    log "[PASS] $name (exited cleanly)"
    PASS_COUNT=$((PASS_COUNT + 1))
    return 0
  fi

  log "[FAIL] $name (exit=$rc)"
  tail -n 40 "$out_file" || true
  FAIL_COUNT=$((FAIL_COUNT + 1))
  return 1
}

log "== Phase 15-17 Validation =="
log "[1/2] Build image"
make NeuroSpark.bin >/dev/null
log "[PASS] Build completed"

log "[2/2] QEMU smoke matrix"
run_smoke "pc_qemu32_localtime" \
  -machine pc -cpu qemu32 -display none -no-reboot -no-shutdown \
  -rtc base=localtime,clock=host -drive file=NeuroSpark.bin,format=raw,index=0,media=disk

run_smoke "q35_default" \
  -machine q35 -display none -no-reboot -no-shutdown \
  -drive file=NeuroSpark.bin,format=raw,index=0,media=disk

run_smoke "i440fx_82" \
  -machine pc-i440fx-8.2 -display none -no-reboot -no-shutdown \
  -drive file=NeuroSpark.bin,format=raw,index=0,media=disk

run_smoke "pc_utc" \
  -machine pc -display none -no-reboot -no-shutdown \
  -rtc base=utc,clock=host -drive file=NeuroSpark.bin,format=raw,index=0,media=disk

{
  echo "Phase 15-17 Validation Report"
  echo "Date: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
  echo "Build: PASS"
  echo "Smoke pass count: $PASS_COUNT"
  echo "Smoke fail count: $FAIL_COUNT"
  if [[ $FAIL_COUNT -eq 0 ]]; then
    echo "Result: PASS"
  else
    echo "Result: FAIL"
  fi
} >"$REPORT_FILE"

rm -rf "$TMP_DIR"

if [[ $FAIL_COUNT -eq 0 ]]; then
  log "== Validation PASS =="
  exit 0
fi

log "== Validation FAIL =="
exit 1
