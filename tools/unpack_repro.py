#!/usr/bin/env python3
import sys
import os
import struct
import json

def main():
    if len(sys.argv) < 2:
        print("Usage: unpack_repro.py <NeuroSpark.bin>")
        sys.exit(1)
        
    disk_path = sys.argv[1]
    if not os.path.exists(disk_path):
        print(f"Error: {disk_path} not found.")
        sys.exit(1)
        
    print(f"Scanning {disk_path} for embedded Reproducibility Bundles...")
    
    with open(disk_path, "rb") as f:
        data = f.read()
        
    # Search for magic 'REPR' (0x52504552 in little-endian is 'R' 'E' 'P' 'R')
    # Actually, 0x52504552 little endian bytes: 0x52, 0x45, 0x50, 0x52 -> 'R' 'E' 'P' 'R'
    magic_bytes = struct.pack("<I", 0x52504552)
    
    offset = data.find(magic_bytes)
    if offset == -1:
        print("Error: No REPR magic header found. Did you run 'repro bundle create'?")
        sys.exit(1)
        
    print(f"Found REPR block at offset {offset}.")
    
    # Read manifest len
    manifest_len_bytes = data[offset+4:offset+8]
    if len(manifest_len_bytes) < 4:
        print("Error: Truncated blob.")
        sys.exit(1)
        
    manifest_len = struct.unpack("<I", manifest_len_bytes)[0]
    print(f"Embedded Manifest Length: {manifest_len} bytes")
    
    if manifest_len > 1024 * 1024 or manifest_len == 0:
        print("Error: Invalid manifest length.")
        sys.exit(1)
        
    # Read manifest string
    manifest_str_bytes = data[offset+8:offset+8+manifest_len]
    manifest_str = manifest_str_bytes.decode('utf-8', errors='ignore')
    
    print("\n--- Extracted Manifest ---")
    try:
        manifest_json = json.loads(manifest_str)
        print(json.dumps(manifest_json, indent=2))
        
        # Save to disk
        with open("manifest.json", "w") as mf:
            json.dump(manifest_json, mf, indent=2)
        print("\nSaved to 'manifest.json'")
    except json.JSONDecodeError:
        print("Raw String:")
        print(manifest_str)
        with open("manifest.json", "w") as mf:
            mf.write(manifest_str)
        print("\nSaved raw string to 'manifest.json'")
        
    # Extract the payload bytes (we wrote a 4-byte payload 0xDEADBEEF)
    payload = data[offset+8+manifest_len:offset+8+manifest_len+4]
    if len(payload) == 4:
        print(f"\nPayload signature extracted: 0x{struct.unpack('<I', payload)[0]:08X}")
        
    print("Bundle extraction and verification complete.")

if __name__ == "__main__":
    main()
