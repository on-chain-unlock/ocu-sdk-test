#!/usr/bin/env python3
"""
inject_config.py — OCU Firmware · Production Configuration Injector
--------------------------------------------------------------------
Replaces @@...@@ placeholders in the compiled binary (libcoresdk.so/CoreSDK.dll)
with real production values, then XOR-obfuscates them so they don't appear
in plaintext with `strings`.

WORKFLOW:
  1. Finds @@PLACEHOLDER@@ patterns in the binary
  2. Overwrites with real values + zero-padding
  3. XOR-encrypts the injected values using SHA256-derived keys
  4. Activates the __XOR_ACTIVE__ marker so the C++ runtime knows to decrypt

The C++ runtime (Storage.hpp) uses the same SHA256(key_name)[:16] derivation
to XOR-decrypt at first access. Values loaded from config files (test.cfg,
config.txt) bypass decryption entirely — the _fromConfig flags prevent it.

USAGE:
  python3 inject_config.py --so libcoresdk.so --config device.cfg
  python3 inject_config.py --so libcoresdk.so \\
      --serial "SN-ABC123" \\
      --key    "AES256KEYHERE..." \\
      --rpc    "https://rpc.matrix.blockchain.enjin.io" \\
      --collection 3333 \\
      --pin    "1234"

  # Skip XOR (debug only — values remain in plaintext):
  python3 inject_config.py --so libcoresdk.so --config device.cfg --no-xor

SECURITY:
  - Always work on a COPY of the .so, never the original.
  - Never commit device.cfg to the repository (add to .gitignore).
  - Values do not appear in source files, build logs, or the final binary.

PLACEHOLDERS (must match Storage.hpp):
  @@SERIAL_NUMBER@@@...  → serial_number  (max 64 bytes)
  @@DEVICE_KEY@@@@...    → device_key     (max 128 bytes)
  @@RPC_URL@@@@@...      → rpc_url        (max 128 bytes)
  @@DEFAULT_PIN@@@...    → default_pin    (max 32 bytes)
  @@COLLECTION_ID@@...   → collection_id  (max 32 bytes)
"""

import argparse
import hashlib
import os
import shutil
import sys

# ─── Placeholder → key name mapping (matches Storage.hpp) ───────────────────
PLACEHOLDERS = {
    b"@@SERIAL_NUMBER@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@":     "serial_number",
    b"@@DEVICE_KEY@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@": "device_key",
    b"@@RPC_URL@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@":    "rpc_url",
    b"@@DEFAULT_PIN@@@@@@@@@@@@@@@@@":                                        "default_pin",
    b"@@COLLECTION_ID@@@@@@@@@@@@@@@@":                                       "collection_id",
}

# Marker che il C++ runtime cerca per sapere se le stringhe sono XOR'd
XOR_MARKER_OFF = b"__XOR_ACTIVE__\x00"
XOR_MARKER_ON  = b"__XOR_ACTIVE__\x01"


# ─── XOR helpers ─────────────────────────────────────────────────────────────

def derive_xor_key(key_name: str, length: int = 16) -> bytes:
    """Derive a per-field XOR key from the field name via SHA256.
    The C++ runtime uses the identical derivation: mbedtls_sha256(name)[:16]."""
    return hashlib.sha256(key_name.encode("ascii")).digest()[:length]


def xor_region(data: bytearray, offset: int, length: int, key: bytes) -> int:
    count = 0
    for i in range(length):
        if data[offset + i] == 0:
            break
        data[offset + i] = ((data[offset + i] - 1 + key[i % len(key)]) % 255) + 1
        count += 1
    return count


# ─── Config file loader ─────────────────────────────────────────────────────

def load_config_file(path: str) -> dict:
    """Reads a key=value file (same format as test.cfg / config.txt)."""
    config = {}
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if "=" not in line:
                continue
            key, _, val = line.partition("=")
            config[key.strip()] = val.strip()
    return config


# ─── Core injection + obfuscation ───────────────────────────────────────────

def inject(so_path: str, values: dict, dry_run: bool = False,
           skip_xor: bool = False) -> bool:
    """
    1. Replace @@placeholders@@ with real values (zero-padded).
    2. XOR-encrypt the injected values (unless --no-xor).
    3. Activate __XOR_ACTIVE__ marker.
    Returns True if at least one placeholder was replaced.
    """
    with open(so_path, "rb") as f:
        data = bytearray(f.read())

    changed = False
    injected_regions = []  # [(offset, value_length, key_name), ...]

    # ── PASS 1: Inject plaintext values ──────────────────────────────────
    for placeholder, key_name in PLACEHOLDERS.items():
        value = values.get(key_name)
        if not value:
            print(f"  [SKIP] {key_name} — not provided")
            continue

        value_bytes = value.encode("ascii")
        ph_len = len(placeholder)
        val_len = len(value_bytes)

        if val_len > ph_len:
            print(f"  [ERROR] '{key_name}': value too long "
                  f"({val_len} > {ph_len} bytes). Extend placeholder and recompile.")
            return False

        pos = data.find(placeholder)
        if pos == -1:
            print(f"  [WARN] {key_name} — placeholder not found "
                  f"(already injected or wrong binary)")
            continue

        padded = value_bytes + b"\x00" * (ph_len - val_len)
        data[pos:pos + ph_len] = padded
        changed = True
        injected_regions.append((pos, val_len, key_name))

        masked = value[:4] + "*" * max(0, len(value) - 4)
        print(f"  [INJ]  {key_name} → '{masked}' ({val_len}/{ph_len} bytes)")

    if not changed:
        print("  No placeholders replaced.")
        return False

    # ── PASS 2: XOR obfuscation ──────────────────────────────────────────
    if not skip_xor and injected_regions:
        xor_count = 0
        for offset, val_len, key_name in injected_regions:
            key = derive_xor_key(key_name)
            n = xor_region(data, offset, val_len, key)
            if n > 0:
                xor_count += 1
                print(f"  [XOR]  {key_name} — {n} bytes encrypted")

        if xor_count > 0:
            marker_pos = data.find(XOR_MARKER_OFF)
            if marker_pos >= 0:
                data[marker_pos + 14] = 0x01
                print(f"  [XOR]  Marker activated at offset {marker_pos}")
            else:
                print(f"  [WARN] __XOR_ACTIVE__ marker not found in binary. "
                      f"Add it to Storage.hpp and recompile.")
    elif skip_xor:
        print(f"  [---]  XOR skipped (--no-xor)")

    # ── Write ────────────────────────────────────────────────────────────
    if not dry_run:
        with open(so_path, "wb") as f:
            f.write(data)
        print(f"\n  Binary updated: {so_path}")
        if not skip_xor and injected_regions:
            print(f"  Strings are XOR-obfuscated — invisible to `strings`")
    else:
        print(f"\n  [DRY RUN] No changes written.")

    return True


# ─── CLI ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Inject + obfuscate production config into CoreSDK binary"
    )
    parser.add_argument("--so", required=True,
                        help="Path to .so or .dll to modify")
    parser.add_argument("--config",
                        help="key=value configuration file")
    parser.add_argument("--serial",      help="serial_number")
    parser.add_argument("--key",         help="device_key")
    parser.add_argument("--rpc",         help="rpc_url")
    parser.add_argument("--collection",  help="collection_id")
    parser.add_argument("--pin",         help="default_pin")
    parser.add_argument("--backup",      action="store_true",
                        help="Create .orig backup before modifying")
    parser.add_argument("--dry-run",     action="store_true",
                        help="Simulate without writing")
    parser.add_argument("--no-xor",      action="store_true",
                        help="Skip XOR obfuscation (debug only)")
    args = parser.parse_args()

    if not os.path.isfile(args.so):
        print(f"ERROR: file not found: {args.so}")
        sys.exit(1)

    values = {}
    if args.config:
        if not os.path.isfile(args.config):
            print(f"ERROR: config not found: {args.config}")
            sys.exit(1)
        values.update(load_config_file(args.config))
        print(f"Configuration from: {args.config}")

    if args.serial:     values["serial_number"] = args.serial
    if args.key:        values["device_key"]    = args.key
    if args.rpc:        values["rpc_url"]       = args.rpc
    if args.collection: values["collection_id"] = args.collection
    if args.pin:        values["default_pin"]   = args.pin

    if not values:
        print("ERROR: no values provided (use --config or --serial/--key/etc.)")
        sys.exit(1)

    if args.backup and not args.dry_run:
        backup = args.so + ".orig"
        shutil.copy2(args.so, backup)
        print(f"Backup: {backup}")

    print(f"\nTarget: {args.so}")
    print("─" * 60)

    ok = inject(args.so, values, dry_run=args.dry_run, skip_xor=args.no_xor)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
