# OCU CoreSDK — On-Chain Unlock
## v1.2.0 — Blockchain-based physical access control

---

### Scope

OCU CoreSDK is designed for residential, SMB, and small-to-medium installations.
The standard $5/device license covers any deployment where scale and criticality
do not require enterprise-grade certification.

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

The trigger is scale + criticality + certification — not the industry sector.
A single ATM in a small branch qualifies for standard licensing.
A national ATM network requires Enterprise.

**Architectural choices that reflect the standard scope:**
- Generic emergency PIN instead of per-user HSM
- Public Enjin RPC endpoint instead of self-hosted node
- Single `.so` / `.dll` instead of microservice architecture
- SQLite-free — encrypted flat files only

---

### Overview

OCU is an embedded access control system that uses NFT ownership on the Enjin
Matrix blockchain as the authentication mechanism. No central server stores
credentials — the source of truth is the chain.

**Stack:**
- **CoreSDK** — C++ shared library (`.so` / `.dll`), the only component the integrator touches
- **Routes.hpp** — HTTP server layer (cpp-httplib in the reference implementation), thin wrapper around the Core API
- **login.h / whitelist.h** — self-contained HTML/CSS/JS frontends, zero external CDN
- **Wallet** — Flutter + Rust + libsodium dedicated signing app (Android/iOS)

---

### Authentication Model

```
Wallet (Flutter/Rust)
  └── sr25519 sign(nonce)
        └── Core_Poll() verifies signature
              └── Matrix RPC: NFT ownership check (MultiTokens::TokenAccounts)
                    └── Relay GPIO pulse → physical access granted
```

Three access roles:

| Role  | Who enters                  | Whitelist required |
|-------|-----------------------------|--------------------|
| ADMIN | NFT token owner only        | No                 |
| GUEST | Addresses in whitelist      | Yes                |
| NONE  | Anyone with a wallet        | No — trace only    |

The NONE route does not grant privileged access — it logs the wallet address
and opens the relay for traceable public events (open house, etc.).

---

### SID Payload Format

The SID is the universal authentication payload — the same bytes go into QR,
NFC (NDEF), or deep link:

```
ocu:[TT][NNNNNNNNNNNNNNNNNNNNNN][LLLLLL...]
     ^^  ^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^
     |   22-char nonce           hex(deviceLabel|routeTitle)
     |   (includes timestamp     split on pipe '|'
     |    in last 10 digits)
     |
     2-hex type byte:
       00 = GUEST  non-persistent
       01 = GUEST  persistent (redirect)
       02 = ADMIN  non-persistent
       03 = ADMIN  persistent (redirect)
       04 = NONE   (trace only)
```

The wallet parses `substring(4,6)` for type, `substring(6,28)` for nonce,
`substring(28)` decoded as UTF-8 and split on `|` for device label and route title.

---

### Vault File Format

Two encrypted files on disk:

| File           | Contents                                    | Key derivation                              |
|----------------|---------------------------------------------|---------------------------------------------|
| `vault.enc`    | admin address + NFT token ID                | SHA256(serial_number + device_key)          |
| `whitelist.enc`| WhitelistEntry[] with temporal constraints  | SHA256(serial_number + token_id + admin_address) |

Both use AES-256-CBC with a random IV (16 bytes from `/dev/urandom`) prepended
to the ciphertext. Format: `[IV(16)][ciphertext]`.

A `device_label.txt` is stored in plaintext alongside the vault — it contains
the human-readable device name (max 20 chars).

---

### Lifecycle

```cpp
Core_Configure(&hw);          // GPIO, EEPROM callbacks
Core_ConfigureStorage(&cfg);  // serial, key, RPC URL, paths
Core_Init();                  // load vault, whitelist, init sessions

Core_RegisterRoute("relay",    CORE_ROLE_GUEST, "DOOR-1",         nullptr, true);
Core_RegisterCommand("relay",  18, 2000);  // GPIO 18, 2s pulse
Core_RegisterRoute("accesses", CORE_ROLE_ADMIN, "ACCESS CONTROL", "/accesscontrol", false);
Core_RegisterRoute("admin",    CORE_ROLE_ADMIN, "ADMIN PANEL",    "/setting",        false);

// ... HTTP server loop ...

Core_Shutdown();
```

---

### API Reference

#### Lifecycle

| Function                    | Description                                          |
|-----------------------------|------------------------------------------------------|
| `Core_Configure(hw)`        | Set GPIO/EEPROM callbacks. NULL = simulation mode    |
| `Core_ConfigureStorage(cfg)`| Inject runtime config. NULL fields are ignored       |
| `Core_Init()`               | Load vault + whitelist, init session manager         |
| `Core_Shutdown()`           | Flush log, cleanup                                   |
| `Core_CleanExpired()`       | Remove stale sessions (call every ~10 min)           |

#### Route Registry

| Function                                              | Description                          |
|-------------------------------------------------------|--------------------------------------|
| `Core_RegisterRoute(pid, role, title, redirect, allow_open)` | Register an access point    |
| `Core_RegisterCommand(pid, pin, ms)`                  | Bind GPIO relay to a route           |
| `Core_GetRoutes()`                                    | List all registered routes           |
| `Core_SetRouteMode(uuid, pid, mode, valid_until)`     | Change route access level at runtime (NORMAL/OPEN/LOCKED) |

#### Auth Flow

| Function                          | Description                                      |
|-----------------------------------|--------------------------------------------------|
| `Core_Start(pid, cookie_uuid)`    | Begin session, returns SID + nonce               |
| `Core_GetSessionPayload(nonce)`   | Retrieve SID for NFC/QR refresh                  |
| `Core_Poll(pid, nonce)`           | Check signature + NFT, fire relay if verified    |
| `Core_Emergency(nonce, pin, pid)` | Offline PIN access (server must be down)         |
| `Core_Logout(uuid)`               | Invalidate session                               |

#### Administration (requires ADMIN session)

| Function                                                     | Description                       |
|--------------------------------------------------------------|-----------------------------------|
| `Core_Admin_GetWhitelist(uuid)`                              | Full whitelist with constraints   |
| `Core_Admin_GetPending(uuid)`                                | Last 10 unauthorized attempts     |
| `Core_Admin_ApproveAddress(uuid, addr, name)`                | Promote pending → whitelist       |
| `Core_Admin_AddAddress(uuid, addr, name, from, to, schedule)`| Direct whitelist injection        |
| `Core_Admin_RemoveAddress(uuid, addr)`                       | Revoke access                     |
| `Core_Admin_UpdatePin(uuid, pin)`                            | Change emergency PIN (min 4 digits)|
| `Core_Admin_GetSecurityStatus(uuid)`                         | PIN status, token ID, failed attempts|
| `Core_Admin_GetLogs(uuid, n)`                                | Last N access log lines           |
| `Core_Admin_Reboot(uuid)`                                    | Async reboot (500ms delay)        |
| `Core_Admin_GetNftInfo()`                                    | Check token minted + owner        |
| `Core_Admin_GetDeviceLabel()`                                | Get device label                  |
| `Core_Admin_SetDeviceLabel(uuid, label)`                     | Set device label, max 20 chars    |

#### Authorization

| Function                      | Description                                          |
|-------------------------------|------------------------------------------------------|
| `Core_GetRole(uuid)`          | Returns CORE_ROLE_ADMIN/GUEST/NONE                   |
| `Core_CheckAccess(uuid, pid)` | Boolean role check against route requirement         |

#### Diagnostics

| Function                        | Description                                        |
|---------------------------------|----------------------------------------------------|
| `Core_GetVersion()`             | SDK version string                                 |
| `Core_IsServerOnline()`         | Gateway reachability                               |
| `Core_GetTokenID()`             | Token ID from EEPROM (0 if not present)            |
| `Core_HandleHardwareReset()`    | Call every ~100ms, handles ≥10s physical reset     |

---

### Emergency PIN Lockout & Offline Handling

Lockout progression after 3 failed attempts:

| Attempt | Wait     |
|---------|----------|
| 3       | 5 min    |
| 4       | 10 min   |
| 5       | 20 min   |
| 6       | 40 min   |
| 7+      | 240 min (cap) |

The counter is reset by:
- A correct emergency PIN entry
- A successful ADMIN blockchain authentication (when online)
- A physical hardware reset via `Core_HandleHardwareReset()` (requires physical access)

The lockout survives reboots (stored on disk). The PIN is stored as
`SHA256(pin + serial_number)` — unique per device, immune to rainbow tables.

---

### Whitelist Entry Schema

```json
{
  "address":    "efXxxx...xxxYYYY",
  "name":       "Mario Rossi",
  "valid_from": 1700000000,
  "valid_to":   1800000000,
  "schedule": [
    { "days": [1,2,3,4,5], "from": "08:00", "to": "18:00" }
  ]
}
```

All fields except `address` are optional and independent. `valid_from`/`valid_to`
are Unix timestamps (0 = no limit). days: 0=Sun … 6=Sat.

---

### Filesystem Layout

On embedded targets it is recommended to separate the program directory from
the data directory.

```
/firmware/                  ← program directory (firmware flash)
    libcoresdk.so.1.2.0     ← CoreSDK shared library
    libcoresdk.so.1         ← symlink → libcoresdk.so.1.2.0
    libcoresdk.so           ← symlink → libcoresdk.so.1
    user.pin                ← PIN hash (32 bytes), written at first PIN set

/data/                      ← data directory (writable partition)
    vault.enc               ← ~200 bytes fixed, admin address + token ID
    whitelist.enc           ← grows with whitelist size
    access_log.txt          ← grows over time, auto-rotated at 10MB
    device_label.txt        ← 20 bytes max, plaintext
    route_locks.json        ← persists LOCKED routes across reboots
    certs/
        cacert.pem          ← CA bundle for HTTPS RPC
```

**Why separate them:**
- The whitelist can grow unbounded — keeping it on firmware flash risks filling a limited partition
- Separating data makes factory reset trivial: wipe `/data/`, firmware is untouched
- `user.pin` stays on firmware flash — harder to tamper with than a data partition
- Log rotation can be applied to `/data/` independently without touching the program

Configuration in `main_with_sdk.cpp`:

```cpp
CoreStorageConfig cfg = {};
cfg.log_path       = "/data/access_log.txt";
cfg.vault_path     = "/data/vault.enc";       // whitelist.enc derives from this path
cfg.ca_bundle_path = "/firmware/certs/cacert.pem";
Core_ConfigureStorage(&cfg);
```

The `user.pin` path is not configurable — it always resolves relative to the
executable. This is intentional: keeping it in the firmware directory makes it
as hard to tamper with as the binary itself.

---

### Security Notes

- **No plaintext keys on disk.** Vault is AES-256-CBC with random IV per write.
- **Signature verification** uses sr25519 via sp_core (Substrate-compatible).
- **Nonce & Clock Dependency:** Nonce = `OCU_` + 8 hex bytes from `/dev/urandom`
  + 10 decimal timestamp digits. Expires in 120s. **CRITICAL:** Nonce validation
  requires a synchronized system clock (NTP or hardware RTC). Clock drift will
  invalidate valid nonces.
- **NFT check** uses `state_getStorage` + `state_getKeys` over HTTPS RPC.
- **EEPROM hot-swap:** removing the EEPROM disables all access including admin.
  Re-insert + transfer NFT to recover.
- **Removable EEPROM as hardware key:** if wallet is compromised, remove EEPROM
  → system locks immediately. Transfer NFT to new wallet → re-insert EEPROM →
  system operational.
- **Token ID** is read exclusively from EEPROM at runtime. It cannot be injected
  into the production binary. In `CORE_TEST_MODE` only, `token_id_override` can
  be set via `test.cfg`.

---

### Multi-Zone Offline Access (integrator pattern)

When the server is offline, Core falls back to PIN mode — a single generic PIN
for the device. For installations requiring room-level access control offline,
an integrator can implement a second authentication layer inside the route action:

```
[offline]

Step 1 — Core PIN (generic)
    Core_Poll() → verified (PIN correct)
    └─ proves: user knows the device PIN

Step 2 — Room PIN (granular, integrator-managed)
    Integrator intercepts verified state
    Redirects to internal keypad / secondary auth
    └─ proves: user knows the specific room secret

Only if both pass → relay fires
```

The Core always acts as the first gate. Neither layer alone is sufficient.
This pattern requires no Core modifications.

---

### Configuration

**test.cfg / config.txt** (key=value, development only):

```
serial_number=ABC123
device_key=mysecretkey
rpc_url=https://rpc.matrix.blockchain.enjin.io
log_path=access_log.txt
collection_id=3333
token_id_override=0    # CORE_TEST_MODE only — silently ignored in production
default_pin=1234
```

In production, values are injected directly into the binary by `inject_config.py`
before flashing. `token_id_override` has no effect in production builds —
the token ID is always read from the EEPROM hardware.

---

### Trustless Architecture

This is the most important security property of the system.

```
NFT on-chain (Enjin Matrix)  ←  source of truth
    └─ sr25519 signature (wallet)
          └─ Core verifies locally on the device
                └─ GPIO relay fires
```

The OCU Server sits outside this chain of trust. Its only role is license
verification and payload relay — it never sees the whitelist, never touches
the hardware, and cannot authorize access on its own.

**What this means in practice:**
- The OCU Server can be compromised, shut down, or sold — no door opens
- The producer cannot open a door they did not install
- A network man-in-the-middle cannot forge a verified access
- Revoking access means transferring the NFT — no server call needed

The server is a trusted courier, not a trusted authority. It can delay or
interrupt service (denial of access) but it cannot grant access that the
chain has not already authorized.

**Honest disclaimer:** these properties hold only if the integrator uses the
CoreSDK correctly and does not introduce backdoors in the firmware or bypass
the signature verification. Auditing the CoreSDK source and the device firmware
is the only way to verify it independently.

**Comparison with traditional access control systems:**

| Property                   | Traditional        | OCU                    |
|----------------------------|--------------------|------------------------|
| Vendor backdoor            | Always present     | Impossible by design   |
| Server compromise = open   | Yes                | No                     |
| Works without internet     | PIN only           | PIN only               |
| Revocation                 | Database entry     | NFT transfer           |
| Audit trail                | Vendor's server    | Public blockchain      |
| Trust required             | Vendor + server    | Chain only             |

---

### Zero-Touch Provisioning (example flow)

End-to-end flow to bring a new device from factory to field.

#### 1. Build and Flash

```bash
# Work on a copy — never modify the original binary
cp libcoresdk.so.1.2.0 libcoresdk_device_001.so

python3 inject_config.py \
  --so     libcoresdk_device_001.so \
  --serial "SN-ABC123" \
  --key    "mysecretkey" \
  --rpc    "https://rpc.matrix.blockchain.enjin.io" \
  --collection 3333 \
  --pin    "1234" \
  --backup
```

Flash `libcoresdk_device_001.so` to the device. The device boots with `Core_Init()`
reading config directly from the binary — no config file needed in production.

#### 2. Create Collection (once per product line)

Create a collection on Enjin Matrixchain using the Enjin Platform or the console
explorer. The collection ID is shared across all devices of the same product line.

- Platform UI (no code): [Quick Start Guide](https://docs.enjin.io)
- GraphQL API: [Creating Tokens](https://docs.enjin.io/docs/minting-a-token)
- Console Explorer: [console.enjin.io](https://console.enjin.io)

To create an NFT (supply = 1), set `cap: collapsing supply = 1` in the mint params.

#### 3. Mint Token (one per device)

Mint one token per device using the Wallet Daemon.

```json
{
  "node": "wss://rpc.matrix.blockchain.enjin.io:443",
  "api":  "https://platform.enjin.io:443/graphql"
}
```

#### 4. Verify Before Shipping

```json
GET /api/admin/nft/info

{
  "status":        "ok",
  "token_id":      999,
  "collection_id": 3333,
  "minted":        true,
  "owner_address": "efXxxx...xxxYYYY"
}
```

`minted: true` + `owner_address` present = token exists on-chain and is assigned.

#### 5. Customer Receives Kit

The customer installs OCU KeyStore, imports or generates a wallet, and receives
the NFT transfer from the producer. From this moment `Core_Poll()` will verify
their wallet as ADMIN.

#### 6. FuelTank (optional — subsidize transfer fees)

Set up a FuelTank with a `WhitelistedCollections` rule for your collection ID.
The producer pays the ENJ fees and the customer receives the NFT without needing ENJ.

- [FuelTank guide](https://docs.enjin.io/guides/platform/managing-users/using-fuel-tanks)

---

### Anti-Hijacking Zero-Touch Provisioning Flow

```
Device (first boot, no vault)
  │
  ├─ registers temporary NONE route: /claim
  │
  │   Adversary scans /claim over network → Rejected: Physical Button Not Pressed
  │
  │   Legitimate Customer presses physical hardware button
  │   Device activates 30-second authorization window
  │
  │   Customer scans /claim QR with wallet app
  │   Signs nonce → Core_Poll() registers address
  │
  ├─ reads address from log
  ├─ calls producer server: POST /provision
  │     { serial, collection_id, token_id, owner_address }
  │
  │   Producer server (Wallet Daemon)
  │   └─ mints token → transfers to owner_address
  │
  ├─ polls Core_Admin_GetNftInfo() until minted == true
  ├─ vault is written (Core now has an ADMIN)
  └─ /claim route removed — provisioning complete
```

```cpp
// Only register /claim if no vault exists yet
if (!Core_Init()) {
    Core_RegisterRoute("claim", CORE_ROLE_NONE, "CLAIM DEVICE", nullptr, false);
    // Physical button required before Core_Poll("claim", nonce) is accepted
}
```

---

### Documentation Links

| Resource             | URL                                                                 |
|----------------------|---------------------------------------------------------------------|
| Enjin Docs           | https://docs.enjin.io                                               |
| Matrixchain overview | https://docs.enjin.io/enjin-blockchain/enjin-matrixchain            |
| MultiToken pallet    | https://docs.enjin.io/docs/multitoken-pallet                        |
| Minting guide        | https://docs.enjin.io/docs/minting-a-token                          |
| FuelTank guide       | https://docs.enjin.io/guides/platform/managing-users/using-fuel-tanks|
| Console explorer     | https://console.enjin.io                                            |
| RPC endpoint         | wss://rpc.matrix.blockchain.enjin.io                                |

---

### Audit Trail

OCU produces two distinct and independent audit trails:

**On-chain audit trail (immutable)**
NFT ownership events — mint, transfer, revocation — are recorded on Enjin
Matrixchain. Public, immutable, verifiable by anyone without trusting the device
or the vendor. This is the cryptographic proof of who owns the access right at
any given time.

**Local access log (private)**
Physical access events are logged locally on the device's storage partition
(`/data/access_log.txt`). This data is private, under the sole control of the
Licensee, and never transits through any cloud services or vendor infrastructure.
Log is automatically rotated at 10MB, keeping the last 2000 lines.

---

*OCU CoreSDK v1.2.0 — On-Chain Unlock — https://on-chain-unlock.net*