#!/usr/bin/env python3
"""
inject_config.py — OCU Firmware · Production Configuration Injector
--------------------------------------------------------------------
Replaces @@...@@ placeholders in the compiled binary (libcoresdk.so)
with real production values, without recompiling.

How it works:
  The compiler leaves static strings from Storage.hpp in the .rodata
  segment of the .so as plain ASCII text. This script locates them and
  overwrites them in-place with the real values, zero-padding up to the
  original placeholder length.

USAGE:
  python3 inject_config.py --so libcoresdk.so.1.2.0 --config device.cfg
  python3 inject_config.py --so libcoresdk.so.1.2.0 \
      --serial "SN-ABC123" \
      --key    "AES256KEYHERE..." \
      --rpc    "https://rpc.matrix.blockchain.enjin.io" \
      --collection 3333 \
      --pin    "1234"

SECURITY:
  - Always work on a COPY of the .so, never the original.
  - Never commit device.cfg to the repository (add to .gitignore).
  - Values do not appear in source files or build logs.

NOTES:
  - The TokenID is read exclusively from the physical EEPROM at runtime.
    The @@TOKEN_ID_OVERRIDE@@ placeholder no longer exists — in production
    the token ID cannot be injected into the binary for security reasons.
  - In CORE_TEST_MODE, token_id_override can be set via test.cfg.

LIMITS:
  - The injected value cannot be longer than the placeholder (@@...@@).
  - Only ASCII strings are supported (no multibyte UTF-8 in values).

RECOGNIZED PLACEHOLDERS:
  @@SERIAL_NUMBER@@  → serial_number  (max 64 bytes)
  @@DEVICE_KEY@@     → device_key     (max 128 bytes)
  @@RPC_URL@@        → rpc_url        (max 128 bytes)
  @@COLLECTION_ID@@  → collection_id  (uint32 as decimal string, max 32 bytes)
  @@DEFAULT_PIN@@    → default_pin    (factory PIN, max 32 bytes)

"""

import argparse
import os
import sys
import shutil


# ---------------------------------------------------------------------------
# Placeholder → argument key mapping
# ---------------------------------------------------------------------------
PLACEHOLDERS = {
    b"@@SERIAL_NUMBER@@":  "serial_number",
    b"@@DEVICE_KEY@@":     "device_key",
    b"@@RPC_URL@@":        "rpc_url",
    b"@@COLLECTION_ID@@":  "collection_id",
    b"@@DEFAULT_PIN@@":    "default_pin",
}


def load_config_file(path: str) -> dict:
    """Reads a key=value file (Storage format, NOT ini-style)."""
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


def inject(so_path: str, values: dict, dry_run: bool = False) -> bool:
    """
    Replaces placeholders in the .so file with the provided values.
    Returns True if at least one placeholder was replaced.
    """
    with open(so_path, "rb") as f:
        data = bytearray(f.read())

    changed = False

    for placeholder, key in PLACEHOLDERS.items():
        value = values.get(key)
        if not value:
            print(f"  [SKIP] {placeholder.decode()} → '{key}' not provided")
            continue

        value_bytes = value.encode("ascii")
        ph_len      = len(placeholder)
        val_len     = len(value_bytes)

        if val_len > ph_len:
            print(
                f"  [ERROR] '{key}': value too long "
                f"({val_len} bytes > {ph_len} bytes placeholder).\n"
                f"          Extend the placeholder in the sources and recompile."
            )
            return False

        pos = data.find(placeholder)
        if pos == -1:
            print(f"  [WARN] {placeholder.decode()} not found in binary "
                  f"(already injected or wrong binary)")
            continue

        # Overwrite: value + zero-padding up to placeholder length
        padded = value_bytes + b'\x00' * (ph_len - val_len)
        data[pos:pos + ph_len] = padded
        changed = True
        masked = value[:4] + "*" * max(0, len(value) - 4)
        print(f"  [OK]   {placeholder.decode()} → '{masked}' ({val_len}/{ph_len} bytes)")

    if not changed:
        print("  No placeholder replaced.")
        return False

    if not dry_run:
        with open(so_path, "wb") as f:
            f.write(data)
        print(f"\n  Binary updated: {so_path}")
    else:
        print("\n  [DRY RUN] No changes written to disk.")

    return True


def main():
    parser = argparse.ArgumentParser(
        description="Inject production configuration into libcoresdk.so"
    )
    parser.add_argument("--so",         required=True,  help="Path to the .so file to modify")
    parser.add_argument("--config",                     help="key=value configuration file")
    parser.add_argument("--serial",                     help="serial_number")
    parser.add_argument("--key",                        help="device_key")
    parser.add_argument("--rpc",                        help="rpc_url")
    parser.add_argument("--collection",  type=str,      help="collection_id (number)")
    parser.add_argument("--pin",         type=str,      help="default_pin (device factory PIN)")
    parser.add_argument("--backup",      action="store_true",
                        help="Create a .so.orig backup before modifying")
    parser.add_argument("--dry-run",     action="store_true",
                        help="Simulate without writing to disk")
    args = parser.parse_args()

    so_path = args.so
    if not os.path.isfile(so_path):
        print(f"ERROR: file not found: {so_path}")
        sys.exit(1)

    values = {}

    if args.config:
        if not os.path.isfile(args.config):
            print(f"ERROR: config not found: {args.config}")
            sys.exit(1)
        values.update(load_config_file(args.config))
        print(f"Configuration from file: {args.config}")

    if args.serial:     values["serial_number"]  = args.serial
    if args.key:        values["device_key"]      = args.key
    if args.rpc:        values["rpc_url"]         = args.rpc
    if args.collection: values["collection_id"]   = args.collection
    if args.pin:        values["default_pin"]     = args.pin

    if not values:
        print("ERROR: no values provided (use --config or --serial/--key/etc.)")
        sys.exit(1)

    if args.backup and not args.dry_run:
        backup_path = so_path + ".orig"
        shutil.copy2(so_path, backup_path)
        print(f"Backup created: {backup_path}")

    print(f"\nTarget: {so_path}")
    print("Injecting placeholders:\n")
    ok = inject(so_path, values, dry_run=args.dry_run)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()