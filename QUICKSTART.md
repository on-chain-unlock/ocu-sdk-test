# OCU CoreSDK — Quick Start Guide

Get a working access point in 15 minutes. This guide covers the minimum steps from zero to a functional door lock with NFT authentication.

---

## Prerequisites

| What | Why |
|------|-----|
| CoreSDK binary (`.so` or `.dll`) | The access control engine |
| Enjin Matrixchain NFT | Your authentication token — [mint one here](https://docs.enjin.io/docs/minting-a-token) |
| OCU KeyStore app | Wallet for signing — download from [on-chain-unlock.net](https://on-chain-unlock.net) |
| A device | Raspberry Pi 4, any ARM64 SBC, or Windows PC for development |

---

## Step 1 — Get the example project

```bash
git clone https://github.com/on-chain-unlock/ocu-sdk-test
cd ocu-sdk-test
```

---

## Step 2 — Configure

Create `test.cfg` in the build output directory:

```ini
rpc_url=https://rpc.matrix.blockchain.enjin.io
collection_id=3333
token_id_override=333
default_pin=1234
log_path=access_log.txt
ca_bundle_path=certs/cacert.pem
```

Replace `collection_id` and `token_id_override` with your NFT's values. Find them on [console.enjin.io](https://console.enjin.io). Serial number and device key are optional in test mode — the SDK uses built-in test credentials. The `ca_bundle_path` is required for HTTPS communication.

---

## Step 3 — Build

### Windows (development)

```powershell
cd ocu-sdk-test
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Linux ARM64 (production)

```bash
cd ocu-sdk-test
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## Step 4 — Run

```bash
./ocu_core
```

You should see:

```
[CORE] SDK v1.3.0 initialized. SN=MY-DEVICE-001 TokenID=333
-------------------------------------
CoreSDK v1.3.0
TokenID:  333
PORT:     8080
-------------------------------------
[SERVER] Running on port 8080 (HTTP)
```

Open `http://localhost:8080/relay` in a browser. You'll see the login page.

---

## Step 5 — Authenticate

1. Open **OCU KeyStore** on your phone
2. Scan the QR code on the login page
3. Sign with the wallet that owns the NFT
4. The page shows **VERIFIED** — access granted

---

## What just happened

```
Your phone (KeyStore)
  └── signed the nonce with sr25519
        └── Core_Poll() verified the signature
              └── Checked NFT ownership on Enjin Matrixchain
                    └── Access granted ✓
```

No server stored your credentials. No database was queried. The blockchain was the only authority.

---

## Step 6 — Add a real door

In `main_with_sdk.cpp`, the relay is already configured:

```cpp
// Register the door route (GUEST = whitelist + admin)
Core_RegisterRoute("relay", CORE_ROLE_GUEST, "DOOR-1", nullptr, true);

// Bind GPIO pin 18 to the relay, pulse for 2 seconds
Core_RegisterCommand("relay", 18, 2000);
```

Change `18` to your actual GPIO pin. Change `2000` to the pulse duration your lock needs.

---

## Step 7 — Add the admin panel

The admin panel is already configured in the example:

```cpp
Core_RegisterRoute("accesses", CORE_ROLE_ADMIN, "ACCESS CONTROL", "/accesscontrol", false);
```

Open `http://localhost:8080/accesses` and sign with the NFT owner wallet. You can:

- **Add addresses** to the whitelist (Section 01)
- **Approve** intercepted wallets (Section 02)
- **Set temporal constraints** — valid from/to, recurring schedules
- **Lock/Open routes** at runtime (Section 04)
- **Update the emergency PIN** (shield icon)
- **Sign & Save** — all changes require your wallet signature

---

## Step 8 — Go to production

### Inject config into the binary

```bash
python3 inject_config.py \
  --so CoreSDK.so \
  --serial "MY-DEVICE-001" \
  --key "my-secret-key-here" \
  --rpc "https://rpc.matrix.blockchain.enjin.io" \
  --collection "3333" \
  --pin "1234"
```

The values are XOR-obfuscated inside the binary — invisible to `strings`.

### Remove test.cfg

In production the binary reads config from itself. Delete `test.cfg` — if it's present, it overrides the injected values.

### Apply security best practices

See `SECURITY_BEST_PRACTICES.md` for the full deployment hardening guide. The essentials:

```bash
# Dedicated user
sudo useradd -r -s /usr/sbin/nologin ocu
sudo usermod -aG gpio,i2c ocu

# Lock down
sudo chown -R ocu:ocu /opt/ocu /data/ocu
sudo chmod 700 /opt/ocu

# Run as ocu
sudo -u ocu /opt/ocu/ocu_core
```

---

## Common scenarios

### "I want open access for a public event"

From the admin panel, Route Control → click **Open** on the relay route. Anyone with a wallet can enter, all wallets are logged. Click **Close** to restore whitelist mode. Remember to **Sign & Save**.

### "I want to lock the door to admin only"

Route Control → click **Lock**. Only the NFT owner can enter. Useful during maintenance. Note: in emergency mode (server down or EEPROM removed), locked routes revert to PIN access like all non-admin routes — the lock is enforced only while the blockchain is reachable.

If you need a door that is **permanently admin-only** (no emergency PIN fallback), register it as a native ADMIN route:

```cpp
Core_RegisterRoute("vault_door", CORE_ROLE_ADMIN, "VAULT", nullptr, false);
Core_RegisterCommand("vault_door", 25, 2000);
```

Native ADMIN routes never open via emergency PIN — if the blockchain is unreachable, the door stays locked. Use this for high-security zones where denial of access is preferable to unauthorized entry.

### "The server is down"

Emergency mode activates automatically. The PIN configured in step 2 (default: `1234`) grants GUEST access. Change it from the admin panel.

### "I lost my phone / wallet compromised"

1. Remove the EEPROM from the device (kill switch)
2. Transfer the NFT to a new wallet on [console.enjin.io](https://console.enjin.io)
3. Re-insert the EEPROM
4. Sign in with the new wallet — you're the new admin

### "I want multiple doors"

Register one route per door:

```cpp
Core_RegisterRoute("front",  CORE_ROLE_GUEST, "FRONT DOOR",  nullptr, true);
Core_RegisterCommand("front", 18, 2000);

Core_RegisterRoute("garage", CORE_ROLE_GUEST, "GARAGE",      nullptr, true);
Core_RegisterCommand("garage", 23, 3000);

Core_RegisterRoute("gate",   CORE_ROLE_NONE,  "MAIN GATE",   nullptr, false);
Core_RegisterCommand("gate",  24, 5000);
```

Each route gets its own login page, GPIO pin, and access level.

---

## Next steps

| Topic | Document |
|-------|----------|
| Full API reference | `README.md` |
| Security hardening | `SECURITY_BEST_PRACTICES.md` |
| Integration example | `main_with_sdk.cpp` |
| Provisioning automation | README → Provisioning Guide |
| Enjin NFT setup | [docs.enjin.io](https://docs.enjin.io) |
| Enjin ecosystem | [enjin.io/ecosystem/on-chain-unlock](https://enjin.io/ecosystem/on-chain-unlock/) |
