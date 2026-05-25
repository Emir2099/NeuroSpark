#!/usr/bin/env python3
"""Inject a host file into NeuroSpark TFS inside a raw disk image.

Usage:
  python3 tools/tfs_inject.py <image> <host_file> [guest_name]
"""

from __future__ import annotations

import math
import os
import struct
import sys

SECTOR_SIZE = 512
TFS_DIR_LBA = 150
TFS_DATA_START = 200
ENTRY_SIZE = 24  # matches current C struct layout: 12s + u32 + u32 + u32
DIR_BYTES = SECTOR_SIZE
MAX_DIR_ENTRIES = DIR_BYTES // ENTRY_SIZE  # practical on-disk capacity (21)
DATA_LBA_START = 256  # avoid low LBAs used by autosave slots
# boot/boot.asm reads 5*127 sectors starting at LBA 1 (through LBA 635)
# into RAM before entering the kernel. Keep injected payloads above that
# reserved boot-read region to avoid kernel image corruption.
KERNEL_RESERVED_END_LBA = 635
SAFE_DATA_LBA_START = 2048


def _read_sector(f, lba: int) -> bytearray:
    f.seek(lba * SECTOR_SIZE)
    data = f.read(SECTOR_SIZE)
    if len(data) != SECTOR_SIZE:
        raise RuntimeError("image too small while reading sector")
    return bytearray(data)


def _write_sector(f, lba: int, data: bytes) -> None:
    if len(data) != SECTOR_SIZE:
        raise ValueError("sector write must be exactly 512 bytes")
    f.seek(lba * SECTOR_SIZE)
    f.write(data)


def _decode_entry(raw: bytes):
    name_raw, lba, size, flags = struct.unpack("<12sIII", raw)
    name = name_raw.split(b"\x00", 1)[0].decode("ascii", errors="ignore")
    return name, lba, size, flags


def _encode_entry(name: str, lba: int, size: int, flags: int) -> bytes:
    name_b = name.encode("ascii", errors="ignore")[:11]
    name_b = name_b + b"\x00" * (12 - len(name_b))
    return struct.pack("<12sIII", name_b, lba, size, flags)


def _load_entries(sec: bytes):
    entries = []
    for i in range(MAX_DIR_ENTRIES):
        off = i * ENTRY_SIZE
        entries.append(_decode_entry(sec[off : off + ENTRY_SIZE]))
    return entries


def _store_entries(sec: bytearray, entries) -> None:
    for i, (name, lba, size, flags) in enumerate(entries):
        off = i * ENTRY_SIZE
        sec[off : off + ENTRY_SIZE] = _encode_entry(name, lba, size, flags)


def _find_last_used_lba(entries) -> int:
    last = SAFE_DATA_LBA_START - 1
    for _name, lba, size, flags in entries:
        if flags != 1 or size == 0:
            continue
        sectors = max(1, int(math.ceil(size / SECTOR_SIZE)))
        end_lba = lba + sectors - 1
        if end_lba > last:
            last = end_lba
    if last < SAFE_DATA_LBA_START - 1:
        return SAFE_DATA_LBA_START - 1
    return last


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: tfs_inject.py <image> <host_file> [guest_name]", file=sys.stderr)
        return 2

    image_path = sys.argv[1]
    host_path = sys.argv[2]
    guest_name = sys.argv[3] if len(sys.argv) >= 4 else os.path.basename(host_path)

    if not os.path.isfile(image_path):
        print(f"error: image not found: {image_path}", file=sys.stderr)
        return 1
    if not os.path.isfile(host_path):
        print(f"error: host file not found: {host_path}", file=sys.stderr)
        return 1

    host_size = os.path.getsize(host_path)
    if host_size <= 0:
        print("error: host file is empty", file=sys.stderr)
        return 1

    with open(image_path, "r+b") as img, open(host_path, "rb") as src:
        image_bytes = os.path.getsize(image_path)
        image_sectors = image_bytes // SECTOR_SIZE

        dir_sector = _read_sector(img, TFS_DIR_LBA)
        entries = _load_entries(dir_sector)

        target_slot = -1
        for i, (name, _lba, _size, flags) in enumerate(entries):
            if flags == 1 and name == guest_name:
                target_slot = i
                break
        if target_slot < 0:
            for i, (_name, _lba, _size, flags) in enumerate(entries):
                if flags != 1:
                    target_slot = i
                    break
        if target_slot < 0:
            print("error: no free TFS directory slot", file=sys.stderr)
            return 1

        sectors_needed = int(math.ceil(host_size / SECTOR_SIZE))
        last_used = _find_last_used_lba(entries)
        start_lba = max(SAFE_DATA_LBA_START, KERNEL_RESERVED_END_LBA + 1, last_used + 1)
        end_lba = start_lba + sectors_needed - 1

        if end_lba >= image_sectors:
            need_bytes = (end_lba + 1) * SECTOR_SIZE
            print(
                f"error: image too small: need at least {need_bytes} bytes for payload",
                file=sys.stderr,
            )
            return 1

        for i in range(sectors_needed):
            chunk = src.read(SECTOR_SIZE)
            if len(chunk) < SECTOR_SIZE:
                chunk = chunk + b"\x00" * (SECTOR_SIZE - len(chunk))
            _write_sector(img, start_lba + i, chunk)

        entries[target_slot] = (guest_name, start_lba, host_size, 1)
        _store_entries(dir_sector, entries)
        _write_sector(img, TFS_DIR_LBA, bytes(dir_sector))

    print(
        f"ok: injected {host_path} as /{guest_name} "
        f"(slot={target_slot}, lba={start_lba}, size={host_size})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
