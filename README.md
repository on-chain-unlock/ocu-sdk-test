# OCU CoreSDK — On-Chain-Unlock
**v1.3.4** — Blockchain-based physical access control

## Changelog

### v1.3.4
- **Fix**: rpc offline state handling and minor fixes

### v1.3.3
- **Security improvements**: general hardening of the core security architecture

### v1.3.2
- **Thread & buffer optimization**: internal thread management and memory buffers enhanced for maximum stability under heavy load

### v1.3.1
- **RPC cert cross-check**: server-signed cert hash (ECDSA P-256) verified against direct RPC connection at every session start
- **CryptoMaze**: anti-debug custom on sr25519 signature verification
- **Auto-recovery**: device resolves cert changes without reboot via poll probe
- `Core_GetOwnershipStatus`: public endpoint for admin ownership flag on login page
- `Security codes updated`: S21 = cross-check failed, S22 = signature invalid
- `Core_RegisterRoute`: rejects reserved pid "whitelist"
- **BREAKING**: RPC URL must be `https://rpc.matrix.blockchain.enjin.io` — custom RPC endpoints are no longer supported (cert cross-check requirement)

### v1.3.0
- **Signed whitelist**: all changes (add, remove, route locks) are staged in a provisional copy and activated only after the admin signs the geometric nonce with their wallet (sr25519)
- `Core_Admin_SaveWhitelist`: starts the `ocu:10` signing session for the provisional whitelist
- `Core_Admin_IsWhitelistDirty`: returns whether the provisional list differs from the active one
- `Core_Admin_DiscardChanges`: reverts the provisional whitelist to the last signed version
- **SecureEnclave v1**: `mprotect`-protected memory pages with shadow verification
- **TLS certificate pinning**: TOFU pinning on the RPC endpoint at boot; mismatch triggers a security alert
- **RPC response verification**: `state_queryStorageAt` cross-checks CID/TID from the blockchain response against enclave-protected values to prevent storage key substitution attacks
- **Post-injection string obfuscation**: `inject_config.py/batch_inject` applies ADD/SUB cipher after injecting config values; runtime deobfuscation uses SHA256-derived keys
- **Security event logging**: coded alerts for debugger detection (S1), timing anomalies (S2), code modification (S3-S4), memory tampering (S11-S14), RPC MITM (S20-S22), TLS certificate mismatch (S30-S31)
- **Vault V3**: a single `vault.enc` stores admin address, NFT token ID, PIN hash, signed whitelist blob, geometric nonce, and whitelist signature, all protected by AES-256-GCM with random IV
- `admin_ownership_lost` flag in security status — warns if the signing admin no longer owns the NFT
- `Core_Poll` and route handlers: 200 instead of 401 for expired sessions (frontend compatibility)
- Frontend: Sign & Save / Discard buttons in header bar; ownership warning alert

### v1.2.3
- `StartSession`: EEPROM check (`tokenId == 0`) moved before the server call — returns `eeprom_missing` + emergency flag without contacting the gateway
- `PollSession`: EEPROM check now takes priority over server status, so a missing EEPROM is detected even when the server is online

### v1.2.2
- `Core_Poll`: log labels `ADMIN`/`GUEST` instead of `ADMIN_NFT`/`GUEST_NFT`
- `Core_Poll`: rejection events logged — `invalid_signature`, `nft_rejected`, `guest_blocked_no_list`
- `ValidateAccess`: whitelisted addresses are no longer added to pending on ADMIN route rejection
- `getAuthorizedSession`: log role reflects actual user role (`actualRole`), not the minimum required by the route

### v1.2.1
- `Core_Emergency`: emergency PIN now activates when EEPROM is missing (`token_id == 0`), regardless of server reachability
- NONE route: real role now detected (ADMIN/GUEST/NONE) — unknown addresses are added to the pending list only
- `Core_GetAddress`: new — returns wallet address associated with a UUID session

### v1.2.0
- `WhitelistEntry`: name, `valid_from` / `valid_to`, recurring schedule
- NONE route: open access with wallet trace logging
- `Core_RegisterRoute`: `allow_open` flag for runtime GUEST→NONE toggle
- `Core_SetRouteMode`: timed open/lock with automatic revert
- `Core_Admin_GetNftInfo`: token existence + owner lookup (no auth required)
- `Core_Admin_GetDeviceLabel` / `SetDeviceLabel`: persistent device name
- SID label encodes `deviceLabel|routeTitle` for wallet display
- AES vault: random IV per write
- Emergency PIN lockout capped at 240 min; reset by correct PIN or ADMIN auth

---

## Scope

OCU CoreSDK is designed for **residential, SMB, and small-to-medium installations**. The standard $5/device license covers deployments where scale and criticality do not require enterprise-grade certification.

**Standard license covers:**
- Homes, apartments, small offices, co-working spaces
- Boutique hospitality (under 50 access points)
- Single ATMs or isolated banking terminals (non-networked)
- Individual safes and storage units in residential or SMB contexts
- Single-site CCTV and surveillance
- Luxury automotive (individual vehicles)

**Enterprise license required when:**
- Scale: 50+ access points or 200+ devices
- Criticality: bank vaults, ATM networks, critical infrastructure
- Certification: FIPS 140-2, PCI-DSS, ISO 27001 or equivalent required
- Architecture: SaaS platforms or multi-tenant deployments built on the SDK

The trigger is **scale + criticality + certification** — not the industry sector. A single ATM in a small branch qualifies for standard licensing. A national ATM network requires Enterprise.

Architectural choices that reflect the standard scope:
- Generic emergency PIN instead of per-user HSM
- Public Enjin RPC endpoint instead of self-hosted node
- Single `.so` / `.dll` instead of microservice architecture
- SQLite-free — encrypted flat files only

---

## Overview

OCU is an embedded access control system that uses NFT ownership on the **Enjin Matrix blockchain** as the authentication mechanism. No central server stores credentials — the source of truth is the chain.

**Stack:**
- `CoreSDK` — C++ shared library (`.so` / `.dll`), the only component the integrator touches
- `Routes.hpp` — HTTP server layer (cpp-httplib in the reference implementation), thin wrapper around the Core API
- `login.h` / `whitelist.h` — self-contained HTML/CSS/JS frontends, zero external CDN
- **Wallet** — Flutter + Rust + libsodium dedicated signing app (Android/iOS)

---

## Authentication Model

```text
Wallet (Flutter/Rust)
  └── sr25519 sign(nonce)
        └── Core_Poll() verifies signature
              └── Matrix RPC: NFT ownership check (MultiTokens::TokenAccounts)
                    └── Relay GPIO pulse → physical access granted
```

Three access roles:

| Role | Who enters | Whitelist required |
|------|------------|-------------------|
| `ADMIN` | NFT token owner only | No |
| `GUEST` | Addresses in whitelist | Yes |
| `NONE` | Anyone with a wallet | No — trace only |

The **NONE** route does not grant privileged access. It logs the wallet address and opens the relay for traceable public events (open house, etc.). `ADMIN` and `GUEST` addresses passing through a `NONE` route are recognized with their real role and are not added to the pending list.

---

## SID Payload Format

The SID is the universal authentication payload — the same bytes go into QR, NFC (NDEF), or BLE.

### Standard sessions (00–04)

```text
ocu:[TT][NNNNNNNNNNNNNNNNNNNNNN][LLLLLL...]
     ^^  ^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^
     |   22-char nonce           hex(deviceLabel|routeTitle)
     |   OCU_ + 8 hex (random)
     |   + 10 decimal digits
     |   (Unix epoch in tenths
     |    of second % 10^10)
     |
     2-hex type byte:
       00 = GUEST  non-persistent
       01 = GUEST  persistent (redirect)
       02 = ADMIN  non-persistent
       03 = ADMIN  persistent (redirect)
       04 = NONE   (trace only)
```

The wallet parses:
- `substring(4,6)` for the type
- `substring(6,28)` for the nonce
- `substring(28)` decoded as UTF-8 and split on `|` for device label and route title

The 10 timestamp digits are `(milliseconds / 100) % 10^10` — tenths of a second, modulo-wrapped to 10 digits. The wallet uses this value to compute the session countdown without any server communication. The modulo wraps every ~31 years.

### Whitelist signature session (10)

Used when the admin commits whitelist changes.

```text
ocu:10
^^^^^^^^^^^^^^^^^
OCU_ + 8 hex (list length, zero-padded)
+ 64 hex (SHA256 hash of serialized entries + route locks)
```

The geometric nonce encodes the list state: entry count + content hash.

The wallet signs this nonce. If the list changes between staging and signing, the nonce will no longer match and the commit is rejected.

This prevents TOCTOU attacks on the whitelist.

---

## Vault File Format (V3)

Single encrypted file on disk.

| File | Contents | Key derivation |
|------|----------|----------------|
| `vault.enc` | admin address, NFT token ID, signed whitelist blob, geometric nonce, whitelist signature, PIN hash | `SHA256(serial_number + device_key + token_id)` |

Vault V3 uses two authenticated encryption layers.

### Outer layer — device vault

The outer layer protects the full device state against cloning and tampering.

Key derivation:

```text
HWKey = SHA256(serial_number + device_key + token_id)
```

Protected JSON fields:

```json
{
  "addr": "admin wallet address",
  "nft": 1234,
  "nonce": "geometric nonce",
  "sig": "sr25519 signature",
  "pinHash": "SHA256(pin + serial_number)",
  "wl": "<base64 encrypted whitelist blob>",
  "magic": "V3"
}
```

### Inner layer — whitelist blob

The whitelist is encrypted independently from the vault metadata.

Key derivation:

```text
IDKey = SHA256(serial_number + nft_token_id + admin_address)
```

Protected JSON fields:

```json
{
  "entries": [...],
  "locks": [...],
  "magic": "WL"
}
```

The encrypted whitelist blob is stored inside the outer vault. There is no separate `whitelist.enc`.

### Encryption format

Both layers use:

- **AES-256-GCM**
- **Random 96-bit IV** generated from `/dev/urandom`
- **128-bit authentication tag**

Binary layout:

```text
[IV(12)][AUTH_TAG(16)][CIPHERTEXT]
```

AES-GCM provides confidentiality, integrity, and authenticity in a single operation. Any modification to the ciphertext, IV, tag, or protected JSON causes decryption failure.

### Atomic signed state

Whitelist entries and route locks are not activated immediately.

Changes are written into a provisional copy and become active only after an administrator signs the geometric nonce through the wallet.

The signed vault is atomic:
- either the entire vault is valid
- or the entire update is rejected

Partial updates are impossible.

### Device label

A plaintext file is stored alongside the vault:

```text
device_label.txt
```

It contains the human-readable device name, up to 20 characters.

---

## Lifecycle

```cpp
Core_Configure(&hw);          // GPIO, EEPROM callbacks
Core_ConfigureStorage(&cfg);   // serial, key, RPC URL, paths
Core_Init();                  // load signed vault, initialize session manager

Core_RegisterRoute("relay",    CORE_ROLE_GUEST, "DOOR-1",         nullptr, true);
Core_RegisterCommand("relay",   18, 2000);  // GPIO 18, 2s pulse
Core_RegisterRoute("accesses", CORE_ROLE_ADMIN, "ACCESS CONTROL", "/accesscontrol", false);
Core_RegisterRoute("admin",    CORE_ROLE_ADMIN, "ADMIN PANEL",    "/setting",        false);

// ... HTTP server loop ...

Core_Shutdown();
```

---

## API Reference

### Lifecycle

| Function | Description |
|----------|-------------|
| `Core_Configure(hw)` | Set GPIO/EEPROM callbacks. `NULL` = simulation mode |
| `Core_ConfigureStorage(cfg)` | Inject runtime config. `NULL` fields are ignored |
| `Core_Init()` | Load signed vault and initialize session manager |
| `Core_Shutdown()` | Flush log, cleanup |
| `Core_CleanExpired()` | Remove stale sessions (call every ~10 min) |

### Route registry

| Function | Description |
|----------|-------------|
| `Core_RegisterRoute(pid, role, title, redirect, allow_open)` | Register an access point |
| `Core_RegisterCommand(pid, pin, ms)` | Bind GPIO relay to a route |
| `Core_GetRoutes()` | List all registered routes (for admin panel) |
| `Core_SetRouteMode(uuid, pid, mode, valid_until)` | Change route access level at runtime: `NORMAL` / `OPEN` / `LOCKED` (requires `allow_open=true`) |

### Auth flow

| Function | Description |
|----------|-------------|
| `Core_Start(pid, cookie_uuid)` | Begin session, returns SID + nonce |
| `Core_GetSessionPayload(nonce)` | Retrieve SID for NFC/QR refresh |
| `Core_Poll(pid, nonce)` | Check signature + NFT, fire relay if verified |
| `Core_Emergency(nonce, pin, pid)` | Emergency PIN access — active when server is unreachable or EEPROM is missing |
| `Core_Logout(uuid)` | Invalidate session |

### Administration (requires ADMIN session)

| Function | Description |
|----------|-------------|
| `Core_Admin_GetWhitelist(uuid)` | Full whitelist with temporal constraints |
| `Core_Admin_GetPending(uuid)` | Last 10 unauthorized attempts |
| `Core_Admin_ApproveAddress(uuid, addr, name)` | Promote pending → whitelist (provisional) |
| `Core_Admin_AddAddress(uuid, addr, name, from, to, schedule)` | Direct whitelist injection (provisional) |
| `Core_Admin_RemoveAddress(uuid, addr)` | Revoke access (provisional) |
| `Core_Admin_SaveWhitelist(uuid, pid)` | Start `ocu:10` signing session — signs and activates provisional list |
| `Core_Admin_IsWhitelistDirty(uuid)` | Returns `{ dirty: true/false }` — provisional ≠ active? |
| `Core_Admin_DiscardChanges(uuid)` | Revert provisional to last signed version |
| `Core_Admin_UpdatePin(uuid, pin)` | Change emergency PIN (min 4 digits) |
| `Core_Admin_GetSecurityStatus(uuid)` | PIN status, token ID, failed attempts, ownership flag |
| `Core_Admin_GetLogs(uuid, n)` | Last N access log lines |
| `Core_Admin_Reboot(uuid)` | Async reboot (500 ms delay) |
| `Core_Admin_GetNftInfo()` | Check token minted + owner (no auth required) |
| `Core_Admin_GetDeviceLabel()` | Get device label (no auth required) |
| `Core_Admin_SetDeviceLabel(uuid, label)` | Set device label (max 20 chars) |

### Authorization

| Function | Description |
|----------|-------------|
| `Core_GetRole(uuid)` | Returns `CORE_ROLE_ADMIN / GUEST / NONE` |
| `Core_CheckAccess(uuid, pid)` | Boolean role check against route requirement |
| `Core_GetAddress(uuid)` | Returns wallet address associated with a UUID session |

### Diagnostics

| Function | Description |
|----------|-------------|
| `Core_GetVersion()` | SDK version string |
| `Core_IsServerOnline()` | Gateway reachability |
| `Core_GetTokenID()` | Token ID from EEPROM or override |
| `Core_HandleHardwareReset()` | Call every ~100 ms, handles ≥10 s physical reset |

---

## Emergency Mode

Emergency mode activates in two equivalent scenarios:
- **Server unreachable** — the gateway cannot be contacted
- **EEPROM missing** — token ID is 0, on-chain verification impossible

In both cases the behavior is identical:
- `GUEST` routes → accessible with PIN only
- `ADMIN` routes → not accessible
- Whitelist → ignored
- Route locks → ignored

### PIN lockout

Lockout progression after 3 failed attempts:

| Attempt | Wait |
|---------|------|
| 3 | 5 min |
| 4 | 10 min |
| 5 | 20 min |
| 6 | 40 min |
| 7+ | **240 min** (cap) |

The counter is reset by:
- a correct emergency PIN entry
- a successful **ADMIN** blockchain authentication

The PIN is stored as `SHA256(pin + serial_number)` — unique per device and immune to rainbow tables.

---

## Whitelist Entry Schema

```json
{
  "address": "efXxxx...xxxYYYY",
  "name": "Mario Rossi",
  "valid_from": 1700000000,
  "valid_to": 1800000000,
  "schedule": [
    { "days": [1,2,3,4,5], "from": "08:00", "to": "18:00" }
  ]
}
```

All fields except `address` are optional and independent.

- `valid_from` / `valid_to` are Unix timestamps (`0` = no limit)
- `days`: `0 = Sun` … `6 = Sat`

Route locks are stored inside the signed whitelist blob as entries shaped like:

```json
{
  "pid": "relay",
  "mode": 2,
  "valid_until": 0
}
```

---

## Filesystem Layout

On embedded targets it is recommended to separate the program directory from the data directory. The program directory lives on firmware flash (effectively read-only at runtime), while the data directory lives on a separate writable partition (eMMC, SD, tmpfs).

```text
/firmware/                  ← program directory (firmware flash)
    ocu_core                ← CoreSDK executable / .so
    certs/
        cacert.pem          ← CA bundle for HTTPS RPC

/data/                      ← data directory (writable partition)
    vault.enc               ← signed vault (whitelist, PIN hash, route locks, admin identity)
    access_log.txt          ← grows over time, auto-rotated at 10MB
    device_label.txt        ← 20 chars max
```

### Why separate them

- The vault can grow with whitelist size — keeping it on firmware flash risks filling a limited partition
- Separating data makes factory reset trivial: wipe `/data/`, firmware is untouched
- Log rotation can be applied to `/data/` independently without touching the program

### Configuration in `main_with_sdk.cpp`

```cpp
CoreStorageConfig cfg = {};
cfg.log_path       = "/data/access_log.txt";
cfg.vault_path     = "/data/vault.enc";
cfg.ca_bundle_path = "/firmware/certs/cacert.pem";
Core_ConfigureStorage(&cfg);
```

---

## Security Notes

- **No plaintext keys on disk.** Vault is AES-256-GCM with random IV per write. Config strings in the binary are XOR-obfuscated post-injection with SHA256-derived keys.
- **Signature verification** uses sr25519 via `sp_core` (Substrate-compatible).
- **Signed whitelist**: all changes are staged provisionally. They become active only after the admin signs the geometric nonce with their wallet. The vault stores the signature; any modification invalidates it.
- **Nonce** = `OCU_` + 8 hex bytes from `/dev/urandom` + 10 decimal digits (Unix epoch in tenths of second, modulo `10^10`). Session expires in 120 s.
- **NFT check** uses `state_queryStorageAt` with response verification — CID and TID extracted from the blockchain response are cross-checked against enclave-protected values. This prevents storage key substitution attacks.
- **TLS certificate pinning** (TOFU) on the RPC endpoint. A mismatch triggers a security alert and blocks communication.
- **SecureEnclave**: protected memory pages with shadow verification.
- **Security logging**: coded alerts (`S1`–`S32`) for debugger detection, timing anomalies, memory tampering, RPC MITM, and TLS certificate changes. Critical alerts advise against rebooting to preserve forensic state.
- **EEPROM hot-swap**: removing the EEPROM triggers emergency mode — admin authentication is disabled, whitelist is ignored, emergency PIN is the only access method. Re-insert the EEPROM to restore full operation.
- **Removable EEPROM as hardware kill switch**: if a wallet is compromised, remove the EEPROM → admin access disabled immediately → PIN mode only → transfer NFT to a new wallet → re-insert EEPROM → system fully operational with the new owner.

---

## Multi-Zone Offline Access (integrator pattern)

When the server is offline, Core falls back to PIN mode — a single generic PIN for the device. For installations requiring room-level access control offline, an integrator can implement a second authentication layer.

The correct architecture is to register the OCU route **without** a hardware command and bind the relay to the second layer only after Core has verified the identity:

```cpp
// OCU route — identity verification only, no GPIO bound
Core_RegisterRoute("auth", CORE_ROLE_GUEST, "AUTH", nullptr, false);
// No Core_RegisterCommand() on this route

// Relay is controlled by the integrator layer
// only after Core has verified the wallet signature
if (Core_CheckAccess(uuid, "auth"))
    integrator_fire_relay(); // second factor check here
```

This way the relay never fires on Core verification alone — it requires both layers to pass:

```text
Step 1 — Pre-Core layer (integrator-managed)
    Physical keypad, badge reader, biometric — any mechanism
    └── if passes → calls Core_Start()

Step 2 — Core OCU (cryptographic gate)
    sr25519 signature + NFT ownership check
    └── if passes → Core_CheckAccess() returns true

Step 3 — Relay
    Only fires if both Step 1 and Step 2 passed
```

Neither layer alone is sufficient. The pre-Core layer proves physical presence or knowledge. The Core layer proves cryptographic ownership.

The room PIN or secondary factor distribution is entirely the integrator's responsibility — the Core has no knowledge of it.

> **Note on wallet security and additional layers:** CoreSDK can be used as a standalone access control layer and provides strong cryptographic guarantees by design.
>
> OCU KeyStore (the wallet) employs Argon2id key derivation and libsodium encryption to protect private keys. Extracting them from a correctly configured smartphone is non-trivial under normal conditions. However, if the smartphone running the wallet is compromised by malicious software, the following risks apply regardless of the wallet's own protections:
>
> - **During signing** — malware with sufficient privileges could intercept the private key in memory at the moment of use
> - **During seed phrase backup** — the mnemonic is necessarily available in memory when displayed; malware with sufficient privileges could access it at this moment
>
> A compromised smartphone may therefore expose the private key itself, enabling an attacker to sign from any device silently and indefinitely. Smartphone hygiene is a critical component of the overall security posture, not an optional consideration.
>
> For installations where this risk must be mitigated architecturally, two complementary approaches are recommended. The first is the use of an additional pre-Core factor: a PIN or physical interaction required directly on the device running CoreSDK — never transiting the smartphone — ensures that wallet compromise alone is not sufficient to gain access. Since this factor exists only on the CoreSDK device, it cannot be extracted remotely. The second is dedicating a clean smartphone used almost exclusively for OCU authentication, minimising the attack surface exposed to third-party applications and reducing the risk of silent compromise.
>
> CoreSDK does not prevent integrators from adding such layers before `Core_Start()` is called; the cryptographic guarantees of the Core remain the final gate regardless of what precedes them.

---

## Configuration

### `test.cfg` / `config.txt` (`key=value`)

```ini
serial_number=ABC123
device_key=mysecretkey
rpc_url=https://rpc.matrix.blockchain.enjin.io
log_path=access_log.txt
collection_id=3333
token_id_override=0
default_pin=1234
```

In production, values are injected directly into the binary by `inject_config.py` before flashing.

---

## Trustless Architecture

This is the most important security property of the system.

```text
NFT on-chain (Enjin Matrix)  ←  source of truth
    └─ sr25519 signature (wallet)
          └─ Core verifies locally on the device
                └─ GPIO relay fires
```

The OCU Server sits **outside** this chain of trust. Its only role is license verification and payload relay — it never sees the whitelist, never touches the hardware, and cannot authorize access on its own.

### What this means in practice

- The OCU Server can be compromised, shut down, or sold — no door opens
- The producer cannot open a door they did not install
- A network man-in-the-middle cannot forge a verified access
- Revoking access means transferring the NFT — no server call needed

The server is a **trusted courier, not a trusted authority**. It can delay or interrupt service (denial of access) but it cannot grant access that the chain has not already authorized.

> **Honest disclaimer:** these properties hold only if the integrator uses the CoreSDK correctly and does not introduce backdoors in the firmware or bypass signature verification. The trustless guarantee is a property of the architecture — it requires correct implementation to be meaningful. Auditing the CoreSDK source and the device firmware is the only way to verify it independently.

### Comparison with traditional access control systems

| Property | Traditional | OCU |
|----------|------------|-----|
| Vendor backdoor | Always present | Impossible by design |
| Server compromise = door open | Yes | No |
| Works without internet | PIN only | PIN only |
| Revocation | Database entry | NFT transfer |
| Audit trail | Vendor's server | Public blockchain |
| Trust required | Vendor + server | Chain only |

Enterprise access control systems costing hundreds of thousands of euros cannot offer this property — the vendor always retains a privileged position. OCU removes that position entirely.

---

## Provisioning Guide

This is the standard process used to prepare a device for deployment.

### 1. Inject Device Configuration

Before flashing, device-specific configuration is injected directly into the compiled CoreSDK binary.

The injector replaces the embedded placeholders with production values:

```text
serial_number
device_key
rpc_url
collection_id
default_pin
```

Example:

```bash
python3 inject_config.py \
  --so libcoresdk.so \
  --serial SN-ABC123 \
  --key DEVICE_KEY \
  --rpc https://rpc.matrix.blockchain.enjin.io \
  --collection 3333 \
  --pin 1234 \
  --backup
```

After injection, the values are automatically obfuscated and the binary is updated in place.

### 2. Batch Provisioning

For manufacturing runs, multiple device binaries can be generated automatically.

CSV mode:

```bash
python3 batch_inject.py \
  --so libcoresdk.so \
  --csv ocu_licenses.csv \
  --rpc https://rpc.matrix.blockchain.enjin.io \
  --collection 3333 \
  --pin 1234 \
  --out ./injected
```

API mode:

```bash
python3 batch_inject.py \
  --so libcoresdk.so \
  --serials serials.txt \
  --api https://access.on-chain-unlock.net \
  --token YOUR_API_KEY \
  --rpc https://rpc.matrix.blockchain.enjin.io \
  --collection 3333 \
  --pin 1234 \
  --out ./injected
```

In API mode, device keys are retrieved directly from the provisioning server and never stored in CSV files.

### 3. Flash Device

Flash the generated binary to the target hardware.

At boot, CoreSDK loads its configuration directly from the embedded binary.

### Security Notes

Injected values are obfuscated before deployment and do not appear in plaintext through standard binary inspection tools such as `strings`.

Configuration data is embedded directly into the firmware image, reducing deployment complexity and eliminating the need for external configuration files in production environments.

### 2. Create collection (once per product line)

Create a collection on Enjin Matrixchain using the Enjin Platform or the console explorer. The collection ID is shared across all devices of the same product line.

- **Platform UI (no code):** Quick Start Guide
- **GraphQL API:** Creating Tokens
- **Low-level extrinsic:** MultiToken Pallet
- **Console Explorer:** `console.enjin.io`

To create an NFT (supply = 1), set the mint parameters so that the token supply collapses to 1.

### 3. Mint token (one per device)

Mint one token per device using the Wallet Daemon. The daemon signs and broadcasts the mint transaction automatically.

```json
{
  "node": "wss://rpc.matrix.blockchain.enjin.io:443",
  "api": "https://platform.enjin.io:443/graphql"
}
```

The daemon wallet holds the producer's key and signs mint + transfer transactions. Keep the seed phrase secure.

### 4. Configure the provisioning server

Set up the producer's provisioning endpoint (`POST /provision`) backed by the Wallet Daemon. The server receives `{ serial, collection_id, token_id, owner_address }` from the device, queries on-chain state independently via RPC, and mints or transfers the NFT accordingly.

How the device authenticates the provisioning request is an implementation choice left to the producer — serial + key, a shared secret, a one-time token, or any other mechanism that fits the deployment model. The Core does not enforce a specific authentication scheme for this step.

### 5. Customer receives kit

The customer installs the OCU Wallet app and generates or imports a wallet. On first boot the device registers a temporary `/claim` route, the customer scans the QR, and the provisioning server handles the rest automatically. From the moment NFT ownership settles on-chain, `Core_Poll()` verifies the wallet as `ADMIN` and the `/claim` route is permanently removed.

### 6. FuelTank (optional — subsidize transfer fees)

To cover ENJ transaction fees for the customer during token transfer, set up a FuelTank with a `WhitelistedCollections` rule for your collection ID. This way the producer pays the fees and the customer receives the NFT without needing ENJ.

- **FuelTank guide:** Using Fuel Tanks
- **FuelTank pallet:** Fuel Tank Pallet
- **Require Token rule:** subsidize only if the user holds a specific NFT — useful for future feature unlocks

### Zero-touch provisioning (example flow)

This is one possible fully automated provisioning flow that requires no out-of-band communication between producer and customer. Each integrator can implement their own variant.

The key insight is that a `CORE_ROLE_NONE` route logs the signing wallet address even without an NFT — it just lets anyone in and records who signed. This can be used to capture the future admin's address automatically.

```text
Device (first boot, no vault)
  │
  ├─ registers temporary NONE route: /claim
  │
  │   Customer opens wallet app, scans /claim QR
  │   Signs nonce → Core_Poll() logs address (NONE_ACCESS)
  │
  ├─ calls Core_GetAddress(uuid) → owner_address
  ├─ calls producer server: POST /provision
  │     { serial, collection_id, token_id, owner_address }
  │
  │   Producer server (Wallet Daemon)
  │   └─ queries on-chain state independently via RPC
  │   └─ three-way decision (see below)
  │
  ├─ firmware polls Core_Admin_GetNftInfo() until owner settled
  ├─ vault is written (Admin Active)
  └─ /claim route removed — provisioning complete
```

#### Server-side decision logic

When the device hits `POST /provision` with `{ serial, collection_id, token_id, owner_address }`, the producer's server independently queries the on-chain state via its own RPC connection before acting — it does not trust the device's reported state.

```text
                        [ POST /provision ]
                        { serial, token_id, owner_address }
                                 │
                    Server queries on-chain state via RPC
                                 │
       ┌─────────────────────────┼─────────────────────────┐
       ▼                         ▼                         ▼
  [ STATE A ]               [ STATE B ]               [ STATE C ]
 Not yet minted         Owned by manufacturer      Already claimed
       │                         │                         │
 MINT to user            TRANSFER to user          REJECT (409)
       │                         │                         │
       ▼                         ▼                         ▼
 HTTP 201 Created           HTTP 200 OK            HTTP 409 Conflict
```

**State A — Not yet minted**  
The token does not exist on-chain. The server mints directly to `owner_address`.

**State B — Pre-minted, held by manufacturer**  
The NFT exists but is still in the manufacturer's treasury wallet. The server transfers it directly to `owner_address`.

**State C — Already owned by a third party**  
The NFT is owned by a wallet that is neither the manufacturer nor the claiming user. This indicates the device is already provisioned or an attacker is attempting a replay. The server rejects immediately.

#### Firmware reaction matrix

While the server processes States A or B, the firmware handles the HTTP responses as follows:

- **On HTTP 409/403 (State C):** the firmware aborts the sequence, purges the temporary `/claim` session, triggers a local hardware error state, and keeps the device unprovisioned
- **On HTTP 200/201 (States A or B):** the firmware enters a secure polling loop using `Core_Admin_GetNftInfo()`. Once NFT ownership has settled on-chain to `owner_address`, it writes the vault and permanently removes the `/claim` route

#### Implementation sketch in `main_with_sdk.cpp`

```cpp
// Only register /claim if no vault exists yet
if (!Core_Init()) {
    Core_RegisterRoute("claim", CORE_ROLE_NONE, "CLAIM DEVICE", nullptr, false);
    // ... start server, wait for NONE_ACCESS log entry ...
    // ... POST /provision to producer server ...
    // ... on 409: abort and signal error ...
    // ... on 200/201: poll GetNftInfo until minted == true, then write vault and restart ...
}
```

The `/claim` route exists only during the provisioning window. Once the vault is written and the NFT is on-chain, the device reboots into normal operation with `ADMIN` authentication active. The route is never registered again.

> **Note:** this is a reference design, not a mandatory pattern. The producer server endpoint, the polling strategy, the claim window duration, and the hardware error signaling are all implementation choices left to the integrator.

---

## Resources

| Resource | URL |
|----------|-----|
| Enjin Docs | `https://docs.enjin.io` |
| Matrixchain overview | `https://docs.enjin.io/enjin-blockchain/enjin-matrixchain` |
| MultiToken pallet | `https://docs.enjin.io/docs/multitoken-pallet` |
| Minting guide | `https://docs.enjin.io/docs/minting-a-token` |
| FuelTank guide | `https://docs.enjin.io/guides/platform/managing-users/using-fuel-tanks` |
| FuelTank pallet | `https://docs.enjin.io/enjin-blockchain/enjin-matrixchain/fuel-tank-pallet` |
| Console explorer | `https://console.enjin.io` |
| RPC endpoint (firmware) | `https://rpc.matrix.blockchain.enjin.io` |
| RPC endpoint (wallet) | `wss://rpc.matrix.blockchain.enjin.io` |
| Canary testnet faucet | `https://docs.enjin.io/getting-started/using-enjin-coin` |

---

## Audit Trail

OCU produces two distinct and independent audit trails.

### On-chain audit trail (immutable)

NFT ownership events — mint, transfer, revocation — are recorded on Enjin Matrixchain. Public, immutable, and verifiable by anyone without trusting the device or the vendor. This is the cryptographic proof of who owns the access right at any given time.

### Local access log (owner-controlled)

Physical access events — who entered, when, and with which role — are logged locally on the device. This log is under the control of the device owner, subject to their retention policy and applicable GDPR obligations. It is not transmitted anywhere by the Core.

### Access log format

```text
[2026-05-16 12:34:56] PID: relay | Addr: efXxxx...xYYY | Status: SUCCESS | TYPE: ADMIN
[2026-05-16 12:35:10] PID: relay | Addr: efXxxx...xZZZ | Status: SUCCESS | TYPE: GUEST
[2026-05-16 12:36:00] PID: relay | Addr: efXxxx...xWWW | Status: SUCCESS | TYPE: NONE_ACCESS
```

Types: `ADMIN`, `GUEST`, `NONE_ACCESS`, `EMERGENCY_PIN`.
