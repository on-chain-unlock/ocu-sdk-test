# OCU CoreSDK — Security Best Practices for Integrators

This document covers deployment hardening for devices running OCU CoreSDK on Linux ARM64. The Core provides cryptographic access control — these practices protect the environment around it.

---

## 1. User Isolation and Privilege Separation

Run CoreSDK under a dedicated unprivileged user that owns exclusive access to GPIO and EEPROM. The integrator's application (web frontend, proxy, API gateway) runs under a separate user with no hardware privileges.

```
┌──────────────────────────────────────────────┐
│  ocu-core (user: ocu)                        │
│  - Owns CoreSDK.so process                   │
│  - Sole member of gpio, i2c groups           │
│  - Listens on 127.0.0.1:8080 (localhost)     │
│  - No SSH access, no login shell             │
└──────────────────┬───────────────────────────┘
                   │ HTTP (localhost only)
┌──────────────────┴───────────────────────────┐
│  integrator-app (user: web)                  │
│  - Reverse proxy (nginx, caddy, custom)      │
│  - Serves frontend on 0.0.0.0:443 (HTTPS)   │
│  - Cannot access GPIO or EEPROM directly     │
│  - Cannot read CoreSDK binary or vault       │
└──────────────────────────────────────────────┘
```

### Setup

```bash
# Create dedicated user with no login shell
sudo useradd -r -s /usr/sbin/nologin -d /opt/ocu ocu

# Add to hardware groups
sudo usermod -aG gpio,i2c ocu

# Lock down the binary and data directories
sudo chown -R ocu:ocu /opt/ocu /data/ocu
sudo chmod 700 /opt/ocu
sudo chmod 700 /data/ocu

# CoreSDK listens on localhost only — the integrator proxies externally
# In main_with_sdk.cpp:
#   cfg.bind_address = "127.0.0.1";
#   cfg.port = 8080;
```

### Why this matters

An attacker who compromises the web-facing application (XSS, RCE, dependency exploit) lands as user `web`. From there they cannot:

- Read or modify the CoreSDK binary (owned by `ocu`, mode 700)
- Access GPIO pins to trigger relays directly
- Read the vault or access log
- Read injected credentials from the binary

Bypassing the SDK requires compromising the `ocu` user or escalating to root. Standard Linux ACLs make this a distinct, harder attack.

---

## 2. CoreSDK Binary Integrity Verification

After injecting configuration with `inject_config.py`, compute a SHA256 hash of the final binary. The integrator's application can embed this hash and verify it at startup before trusting the Core.

### At build time

```bash
# Inject config
python3 inject_config.py --so CoreSDK.so --serial "..." --key "..." ...

# Compute integrity hash
sha256sum CoreSDK.so | awk '{print $1}' > CoreSDK.so.sha256
```

### At runtime (integrator application)

```cpp
// Read expected hash from a secure location (embedded in the integrator binary,
// or from a read-only file on the firmware partition)
std::string expected = loadExpectedHash();

// Compute actual hash of the CoreSDK binary on disk
std::string actual = sha256File("/opt/ocu/CoreSDK.so");

if (actual != expected) {
    // Binary has been modified — refuse to start
    syslog(LOG_CRIT, "CoreSDK binary integrity check FAILED");
    exit(1);
}

// Safe to dlopen and call Core_Init()
```

### What this protects against

- Attacker replaces the `.so` with a patched version that disables NFT checks
- Attacker modifies injected credentials to point to a different RPC endpoint
- Supply chain tampering between build and deployment

This is complementary to the Core's internal `.text` hash verification — the integrator verifies the whole binary from outside, the Core verifies its own code section from inside.

> **Note:** After any OTA update that replaces the CoreSDK binary, the integrity hash must be recomputed and distributed alongside the update package.
---

## 3. EEPROM Physical Security

The EEPROM stores the Token ID — the link between the physical device and the on-chain NFT. Physical access to the EEPROM means control over which NFT the device trusts.

### Recommendations

**Tamper-evident enclosure.** Mount the EEPROM on the main board inside a sealed enclosure. Use tamper-evident screws or adhesive seals that show visible damage when opened.

**Keyed access slot.** If the EEPROM must be field-replaceable (for NFT transfer scenarios), use a keyed slot accessible only with a physical tool (hex key, proprietary tool). Do not expose the EEPROM on an externally accessible header.

**I2C bus isolation.** The EEPROM I2C bus should not be shared with other peripherals. If the board exposes I2C headers for expansion, use a separate bus for the EEPROM or add a hardware write-protect jumper.

**Write protection.** Most I2C EEPROMs (AT24Cxx) have a WP (Write Protect) pin. Tie WP high after programming to prevent software writes. The Core only reads the Token ID — it never writes to the EEPROM.

```
EEPROM AT24C02
  ┌─────┐
  │ VCC ├── 3.3V
  │ GND ├── GND
  │ SDA ├── I2C SDA (dedicated bus)
  │ SCL ├── I2C SCL (dedicated bus)
  │ WP  ├── VCC (write protected after programming)
  └─────┘
```

---

## 4. GPIO Isolation

GPIO pins that control physical relays (door locks, gates) are the final link in the access chain. If an attacker can toggle GPIO directly, all cryptographic verification is bypassed.

### Restrict GPIO access to the ocu user

```bash
# udev rule: only the ocu user can access GPIO
echo 'SUBSYSTEM=="gpio", KERNEL=="gpiochip*", OWNER="ocu", MODE="0600"' \
    | sudo tee /etc/udev/rules.d/99-ocu-gpio.rules

# For sysfs-based GPIO (legacy):
echo 'SUBSYSTEM=="gpio", ACTION=="add", OWNER="ocu", MODE="0600"' \
    | sudo tee -a /etc/udev/rules.d/99-ocu-gpio.rules

sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Disable unused GPIO exports

Do not export GPIO pins that are not in use. The CoreSDK exports only the pins registered with `Core_RegisterCommand()`. Verify no other process exports additional pins:

```bash
# List currently exported GPIOs
ls /sys/class/gpio/ | grep gpio

# Unexport anything not needed
echo 18 > /sys/class/gpio/unexport  # only if pin 18 shouldn't be exported
```

### Hardware-level isolation

For high-security deployments, add an optocoupler or dedicated relay driver IC between the GPIO pin and the relay. This prevents electrical fault injection on the GPIO bus from triggering the relay.

---

## 5. SSH Hardening

SSH is the primary remote attack vector on embedded Linux devices.

### Disable password authentication

```bash
# /etc/ssh/sshd_config
PasswordAuthentication no
ChallengeResponseAuthentication no
PermitRootLogin no
AllowUsers deployer        # only the deployment user, NOT ocu
MaxAuthTries 3
LoginGraceTime 30
```

### Use key-based authentication only

```bash
# On the deployment machine
ssh-keygen -t ed25519 -f ~/.ssh/ocu_deploy

# Copy to device
ssh-copy-id -i ~/.ssh/ocu_deploy.pub deployer@device

# Verify, then disable password auth
```

### Restrict SSH to a management network

```bash
# /etc/ssh/sshd_config
ListenAddress 192.168.1.1   # management interface only, not the public network
Port 2222                    # non-standard port reduces noise
```

### Fail2ban

```bash
sudo apt install fail2ban

# /etc/fail2ban/jail.local
[sshd]
enabled = true
port = 2222
maxretry = 3
bantime = 3600
findtime = 600
```

### Consider disabling SSH entirely in production

If the device does not require remote access after deployment, disable SSH completely:

```bash
sudo systemctl disable ssh
sudo systemctl stop ssh
```

Use a physical UART console for emergency recovery.

---

## 6. Network Hardening

### Firewall

Allow only the ports the device needs:

```bash
# iptables — allow only HTTPS (frontend) and established connections
sudo iptables -P INPUT DROP
sudo iptables -P FORWARD DROP
sudo iptables -P OUTPUT ACCEPT

sudo iptables -A INPUT -i lo -j ACCEPT
sudo iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 443 -j ACCEPT    # HTTPS frontend
sudo iptables -A INPUT -p tcp --dport 2222 -s 192.168.1.0/24 -j ACCEPT  # SSH from mgmt only

# Make persistent
sudo apt install iptables-persistent
sudo netfilter-persistent save
```

### CoreSDK on localhost only

The CoreSDK HTTP server must never be exposed directly to the network. It listens on `127.0.0.1:8080` and the integrator's reverse proxy handles TLS termination and external access.

```
Internet → nginx (443, TLS) → localhost:8080 (CoreSDK, plain HTTP)
```

### Disable unnecessary services

```bash
# List running services
systemctl list-units --type=service --state=running

# Disable anything not needed (common on embedded images)
sudo systemctl disable avahi-daemon bluetooth cups
```

---

## 7. Filesystem Hardening

### Read-only firmware partition

Mount the firmware partition as read-only after deployment:

```bash
# /etc/fstab
/dev/mmcblk0p1  /firmware  ext4  ro,noatime,nodev,nosuid  0 0
/dev/mmcblk0p2  /data      ext4  rw,noatime,nodev,nosuid,noexec  0 0
```

The `noexec` flag on `/data` prevents execution of any binary placed in the data partition — even if an attacker writes a file there, it cannot be run.

### Restrict vault and log permissions

```bash
chmod 600 /data/ocu/vault.enc
chmod 600 /data/ocu/access_log.txt
chown ocu:ocu /data/ocu/*
```

---

## 8. Automatic Updates and Recovery

### Signed firmware updates

If the device supports OTA updates, sign the update package with an ed25519 key and verify before applying:

```bash
# Build side
minisign -Sm firmware_update.tar.gz -s producer_secret.key

# Device side
minisign -Vm firmware_update.tar.gz -p producer_public.key || exit 1
tar xzf firmware_update.tar.gz -C /firmware/
```

### Watchdog

Enable the hardware watchdog to recover from crashes:

```bash
# /etc/systemd/system/ocu-core.service
[Service]
Type=simple
User=ocu
ExecStart=/opt/ocu/ocu_core
WatchdogSec=30
Restart=always
RestartSec=5
```

If the CoreSDK process hangs or crashes, systemd restarts it within 5 seconds. If the entire system hangs, the hardware watchdog reboots after 30 seconds.
---

## 9. TrustZone Integration (Beta)

On ARM64 targets with TrustZone support (Cortex-A53/A55/A72), CoreSDK delegates the storage of on-chain verification parameters to the Secure World. The following values are held inside the Trusted Execution Environment:

- **Token ID** — which NFT the device trusts
- **Collection ID** — which collection the token belongs to
- **RPC URL** — which blockchain endpoint is contacted for ownership checks
- **PIN hash** — emergency access credential, updated via `Core_Admin_UpdatePin`

These four values are what the Core needs to verify NFT ownership on-chain and authenticate emergency access. By placing them in TrustZone, a fully compromised Linux OS cannot redirect the device to a different RPC endpoint, substitute a different token, point to a different collection, or extract the PIN hash to brute-force the emergency PIN offline.

> **This feature is included in CoreSDK but currently in beta.**
>
> If you encounter any issue — boot failures, TA loading errors, verification mismatches, or unexpected behavior — please report it to the OCU team with your board model, OP-TEE version, and kernel version.

## Summary Checklist

| # | Practice | Priority |
|---|----------|----------|
| 1 | Dedicated `ocu` user with exclusive GPIO/I2C access | **Critical** |
| 2 | CoreSDK listens on localhost only, integrator proxies externally | **Critical** |
| 3 | Binary integrity hash verified at startup | High |
| 4 | EEPROM write-protected and physically secured | High |
| 5 | GPIO restricted to `ocu` user via udev rules | **Critical** |
| 6 | SSH disabled or key-only on management interface | **Critical** |
| 7 | Firewall drops all except 443 and management SSH | High |
| 8 | Firmware partition mounted read-only | High |
| 9 | Data partition mounted with `noexec` | Medium |
| 10 | Watchdog and automatic restart configured | Medium |
| 11 | Signed OTA updates | Medium |
| 12 | Unnecessary services disabled | Medium |
| 13 | TrustZone TEE (beta) | Optional |

---

*This document is intended for integrators deploying OCU CoreSDK on embedded Linux targets. It does not cover Windows development environments. For questions or enterprise-specific hardening requirements, contact the OCU team.*
