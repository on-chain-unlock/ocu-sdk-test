/**
 * CoreSDK.h  —  On-Chain Unlock · Core Library  v1.3.1
 * -------------------------------------------------------
 * Single public header. The integrator includes only this file.
 * All internals (Logic, Storage, SecureVault, Hardware, SessionManager)
 * remain hidden inside the .so / .dll.
 *
 * extern "C" interface — stable ABI: compatible with any C++ compiler
 * and with Python / Go / Rust / NFC bridge bindings.
 *
 * MANDATORY LIFECYCLE:
 *   1. Core_Configure()        — hardware callbacks (GPIO, EEPROM)
 *   2. Core_ConfigureStorage() — file paths, RPC URL, runtime params
 *   3. Core_Init()             — load config, EEPROM token ID, encrypted whitelist
 *   4. Core_RegisterRoute()    — declare each access point (pid, role, title)
 *   5. Core_RegisterCommand()  — bind GPIO relay to a route (optional)
 *   6. ... API calls ...
 *   7. Core_Shutdown()         — flush log, cleanup
 *
 * RESPONSE BUFFERS:
 *   CoreResult.json_payload points to internal static buffers (one dedicated
 *   slot per function, internally mutex-protected). Valid until the next call
 *   to the SAME function. Copy the payload if you need it beyond the current
 *   response.
 *
 * AUTHENTICATION MODEL:
 *   Wallet (Flutter/Rust) signs the nonce with sr25519.
 *   Core_Poll() verifies the signature, then checks NFT ownership on the
 *   Enjin Matrix blockchain (MultiTokens pallet, HTTPS RPC).
 *   Three access roles:
 *     ADMIN — NFT token owner only
 *     GUEST — addresses in the encrypted whitelist
 *     NONE  — anyone with a wallet (trace only, no whitelist check)
 *
 * SID PAYLOAD FORMAT:
 *   "ocu:[TT][22-char nonce][hex(deviceLabel|routeTitle)]"
 *   TT (2 hex): 00=GUEST 01=GUEST+redirect 02=ADMIN 03=ADMIN+redirect 04=NONE
 *   Same payload used for QR image, NFC NDEF record, and DeepLink.
 *
 *  WHAT'S NEW IN v1.3.1: 
 *  - RPC cert cross-check: server-signed cert hash (ECDSA P-256) verified * against direct RPC connection at every session start 
 *  - Auto-recovery: device resolves cert changes without reboot via poll probe
 *  - Security codes updated: S21 = cross-check failed, S22 = signature invalid
 *  - Core_GetOwnershipStatus: public endpoint for admin ownership flag
 * 
 *    NOTE (v1.3.1): The RPC URL must be https://rpc.matrix.blockchain.enjin.io
 *    Cert cross-check requires the device and the OCU gateway to use the same
 *    RPC endpoint. Do not change this value.
 * 
 * WHITELIST MODEL (v1.3.0):
 *   All whitelist modifications (add, remove, route locks) are staged in a
 *   provisional copy. Changes become active ONLY after the admin signs the
 *   full list with their wallet (sr25519 over a geometric nonce).
 *   The signed whitelist is stored in an encrypted vault (AES-256-CBC).
 *   Core_Admin_DiscardChanges() reverts the provisional copy to the active one.
 *
 * WHAT'S NEW IN v1.3.0:
 *   - Signed whitelist: all changes staged, activated only after wallet signature
 *   - Core_Admin_SaveWhitelist: starts ocu:10 signing session
 *   - Core_Admin_IsWhitelistDirty: provisional != active?
 *   - Core_Admin_DiscardChanges: revert provisional to active
 *   - SecureEnclave: mprotect'd memory pages with shadow verification
 *   - TLS certificate pinning on RPC endpoint (TOFU)
 *   - RPC verification: state_queryStorageAt with CID/TID cross-check
 *   - Post-injection XOR obfuscation of config strings
 *   - Security alerts: S1-S32 coded events (debugger, tamper, MITM)
 *   - Route mode changes staged (Sign & Save required)
 *   - admin_ownership_lost flag in security status
 *
 * Changes v1.2.3:
 *   - StartSession: EEPROM check (tokenId==0) moved before the server call —
 *     returns "eeprom_missing" + emergency flag without contacting the gateway
 *   - PollSession: EEPROM check now takes priority over server status,
 *     so a missing EEPROM is detected even when the server is online
 * WHAT'S NEW IN v1.2.2:
 *   - Core_Poll: log labels ADMIN/GUEST instead of ADMIN_NFT/GUEST_NFT
 *   - Core_Poll: rejection events logged (invalid_signature,
 *     nft_rejected, guest_blocked_no_list)
 *   - ValidateAccess: whitelisted addresses no longer added to pending
 *     on ADMIN route rejection
 *   - getAuthorizedSession: log role reflects actual user role
 *     (actualRole) not minimum required by route
 *
 * WHAT'S NEW IN v1.2.1:
 *   - Core_Emergency: emergency PIN now activates when EEPROM is missing
 *     (token_id == 0), regardless of server reachability
 *   - ValidateAccess: NONE route now detects real role (ADMIN/GUEST/NONE),
 *     adds unknown addresses to pending list only
 *   - Core_GetAddress: returns wallet address associated with a UUID session
 *
 * WHAT'S NEW IN v1.2.0:
 *   - WhitelistEntry: name (max 20 chars), valid_from/valid_to, recurring schedule
 *   - NONE route: open access with wallet trace logging
 *   - Core_RegisterRoute: allow_open flag for runtime GUEST→NONE toggle
 *   - Core_SetRouteOpen: timed open door with automatic revert
 *   - Core_Admin_GetNftInfo: token existence + owner lookup (no auth required)
 *   - Core_Admin_GetDeviceLabel / SetDeviceLabel: persistent device name
 *   - SID label now encodes "deviceLabel|routeTitle" for wallet display
 *   - AES vault: random IV per write (from /dev/urandom), no fixed IV
 *   - Emergency PIN lockout capped at 240 min; reset by correct PIN or ADMIN auth
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// --- EXPORT MACRO ---
#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef CORE_SDK_EXPORTS 
    #define SDK_API __declspec(dllexport)
  #else
    #define SDK_API __declspec(dllimport)
  #endif
#else
  #if __GNUC__ >= 4
    #define SDK_API __attribute__((visibility("default")))
  #else
    #define SDK_API
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * SECTION 1 — PUBLIC TYPES
 * ========================================================================= */

/** Generic result. http_code follows HTTP semantics. */
typedef struct {
    int         http_code;
    const char* json_payload;
} CoreResult;

/**
 * Hardware configuration.
 * All callbacks are optional: NULL = default behavior.
 *   gpio_read   == NULL → DigitalRead always returns HIGH (1)
 *   gpio_write  == NULL → no-op with stderr log
 *   eeprom_read == NULL → use token_id_override from CoreStorageConfig
 */
typedef struct {
    int      (*gpio_read)(int pin);
    void     (*gpio_write)(int pin, int value);
    uint32_t (*eeprom_read)(uint8_t i2c_addr, uint8_t reg);
    const char* i2c_bus;       /* Default: "/dev/i2c-1" */
    uint8_t  eeprom_i2c_addr;  /* Default: 0x50         */
    uint8_t  eeprom_token_reg; /* Default: 0x00         */
} CoreHardwareConfig;

/**
 * Storage configuration — injected at runtime via Core_ConfigureStorage().
 *
 * In production these values are written into binary placeholders by
 * inject_config.py before flashing. In development they are read from
 * test.cfg or passed directly here.
 *
 * NULL or zero fields are ignored (Core keeps its current defaults).
 */
typedef struct {
    const char* serial_number;     /* Device serial number                    */
    const char* device_key;        /* AES vault encryption key                */
    const char* log_path;          /* e.g. "access_log.txt"                   */
    uint32_t    log_max_bytes;     /* log rotation threshold in bytes (0 = use default 10MB) */
    int         log_keep_lines;    /* lines to keep after rotation (0 = use default 2000)    */
    const char* rpc_url;           /* Enjin Matrix RPC URL                    */
    const char* ca_bundle_path;    /* e.g. "certs/cacert.pem"                 */
    uint32_t    collection_id;     /* NFT collection ID (0 = use default)     */
    uint32_t    token_id_override; /* 0 = read from EEPROM                    */
    const char* default_pin;       /* Factory PIN (will be hashed with SN)    */
} CoreStorageConfig;

typedef enum {
    CORE_ROLE_UNKNOWN = -1,  /* session absent, expired or not authenticated */
    CORE_ROLE_NONE    =  0,  /* active NONE session (persistent with redirect) */
    CORE_ROLE_GUEST   =  1,  /* NFT holder OR in the whitelist */
    CORE_ROLE_ADMIN   =  2,  /* NFT token owner only */
} CoreRole;


/* =========================================================================
 * SECTION 2 — LIFECYCLE
 * ========================================================================= */

/**
 * Core_Configure — sets hardware callbacks.
 * Call BEFORE Core_Init().
 * Pass NULL to use all defaults (test/simulation mode).
 */
void Core_Configure(const CoreHardwareConfig* hw);

/**
 * Core_ConfigureStorage — injects Storage configuration.
 * Call BEFORE Core_Init().
 *
 * Configuration priority order:
 *   1. Values injected by Python script into the binary (production)
 *   2. This function, if called explicitly
 *   3. "test.cfg" in working directory (development)
 *   4. "config.txt" in working directory (fallback)
 *   5. SDK hardcoded defaults
 *
 * NULL/zero fields do not overwrite values already set.
 */
void Core_ConfigureStorage(const CoreStorageConfig* cfg);

/**
 * Core_Init — full system initialization.
 *   - Resolves configuration (see priority order above)
 *   - Reads TokenID from EEPROM or override
 *   - Decrypts and loads whitelist (vault.enc + whitelist.enc)
 *   - Initializes SessionManager
 * Returns false on critical error (check stderr).
 *
 * NOTE: vault.enc and whitelist.enc are managed exclusively by the Core.
 *       Do not create or modify a plaintext whitelist.txt.
 */
bool Core_Init(void);

/** Core_Shutdown — flush log, cleanup. Call before terminating the process. */
void Core_Shutdown(void);

/**
 * Core_CleanExpired — removes expired sessions from RAM.
 * Call periodically (e.g. every 10 minutes) from the server maintenance loop.
 * Anonymous sessions (not yet authenticated): expire after 120s.
 * Authenticated sessions: expire after 60 minutes of inactivity.
 */
void Core_CleanExpired(void);


/* =========================================================================
 * SECTION 3 — ROUTE AND COMMAND REGISTRY
 * ========================================================================= */

/**
 * Core_RegisterRoute — registers a controlled access point.
 *
 * pid:          Unique identifier (e.g. "relay", "entrance").
 * role:         Minimum required role.
 * title:        Human-readable title (e.g. "MAIN GATE").
 * redirect_url: Post-auth redirect URL. NULL or "" = no redirect.
 * allow_open:   If true, this route can be toggled at runtime with
 *               Core_SetRouteOpen() (useful for physical relays, gates).
 *               Set false for routes that must never be open to everyone
 *               (cameras, control panels, etc.).
 *               Only affects GUEST routes — ADMIN routes are always
 *               protected and cannot be opened.
 * RESERVED: "whitelist" is used internally for signed whitelist commits. Do not register a route with this pid.
 *
 * Returns false if pid is empty or already registered.
 */
bool Core_RegisterRoute(const char* pid,
                        CoreRole    role,
                        const char* title,
                        const char* redirect_url,
                        bool        allow_open);

/**
 * Core_RegisterCommand — binds a route to the GPIO relay it should trigger.
 *
 * pid:         Route PID (must match a registered route).
 * relay_pin:   GPIO pin of the relay to pulse.
 * duration_ms: Pulse duration in milliseconds.
 *
 * The relay is fired automatically and internally by Core_Poll() and
 * Core_Emergency() when access is verified.
 * There is no public function to trigger hardware manually — by design.
 * The relay responds ONLY to a Core-verified access, never to an
 * arbitrary external call.
 */
void Core_RegisterCommand(const char* pid, int relay_pin, int duration_ms);

/**
 * Core_GetRoutes — exposes the registered route list to the server.
 *
 * Returns a JSON array with the information needed by the server to
 * generate HTML pages. Does NOT expose internals (action, hardware).
 *
 * JSON response:
 * {
 *   "routes": [
 *     {
 *       "pid":        "<pid>",
 *       "title":      "<title>",
 *       "redirect":   "<url>" | "NONE",
 *       "role":       "admin" | "guest" | "none",
 *       "is_admin":   bool,
 *       "allow_open": bool
 *     },
 *     ...
 *   ]
 * }
 *
 * Call after Core_Init() and after all routes are registered.
 */
CoreResult Core_GetRoutes(void);

/** Runtime route mode — used by Core_SetRouteMode(). */
typedef enum {
    CORE_ROUTE_NORMAL = 0,  /* Restore registered role (GUEST)                    */
    CORE_ROUTE_OPEN   = 1,  /* GUEST → NONE: anyone who signs enters (trace only) */
    CORE_ROUTE_LOCKED = 2,  /* GUEST → ADMIN: only NFT owner can enter            */
} CoreRouteMode;

/**
 * Core_SetRouteMode — changes a route's access level at runtime.
 * RAM only: reverts to registered role on reboot.
 *
 * Route must have been registered with allow_open=true.
 * OPEN and LOCKED modes accept a valid_until timestamp for automatic revert.
 *
 * uuid:        admin "session_uuid" cookie.
 * pid:         PID of the route to modify.
 * mode:        CORE_ROUTE_NORMAL → restore to registered role (GUEST)
 *              CORE_ROUTE_OPEN   → GUEST becomes NONE (anyone enters, wallet logged)
 *              CORE_ROUTE_LOCKED → GUEST becomes ADMIN (NFT owner only)
 * valid_until: Unix timestamp for automatic revert (0 = no auto-revert).
 *              When expired, route reverts to NORMAL automatically.
 *
 * JSON: { "status": "ok" | "forbidden" | "not_found" |
 *                   "not_applicable" | "open_not_allowed" }
 *   not_applicable   → route role is ADMIN
 *   open_not_allowed → route was not registered with allow_open=true
 */
CoreResult Core_SetRouteMode(const char* uuid,
                              const char* pid,
                              CoreRouteMode mode,
                              int64_t     valid_until);


/* =========================================================================
 * SECTION 4 — AUTHENTICATION FLOW
 * ========================================================================= */

/**
 * Core_Start — begins an authentication session.
 *
 * pid:         PID of the requested route.
 * cookie_uuid: Value of the "session_uuid" cookie. Pass "" for a new session.
 *
 * JSON response:
 * {
 *   "status":       "online" | "offline" | "eeprom_missing",
 *   "sid":          "<session_id>",   ← CANONICAL PAYLOAD for NFC/QR/DeepLink
 *   "nonce":        "<nonce>",        ← sid alias, used internally
 *   "admin_locked": true | false,
 *   "session_uuid": "<uuid>"          ← set as HttpOnly cookie
 * }
 *
 * THE SID IS THE UNIVERSAL PAYLOAD:
 *   The server uses the SID to build the presentation payload for any
 *   channel (QR, NFC, DeepLink) without depending on the Core for media
 *   generation. Examples:
 *     QR  → generate PNG from SID with any library
 *     NFC → write SID as NDEF text record
 */
CoreResult Core_Start(const char* pid, const char* cookie_uuid);

/**
 * Core_GetSessionPayload — retrieves SID and nonce of an active session.
 *
 * nonce: nonce received from Core_Start (frontend query parameter).
 *
 * JSON response:
 * {
 *   "status": "ok" | "expired",
 *   "sid":    "<session_id>",   ← payload for NFC/QR/DeepLink
 *   "nonce":  "<nonce>"
 * }
 *
 * Use to regenerate the payload on demand (e.g. expired QR refresh,
 * new NFC write, etc.) without restarting the session.
 */
CoreResult Core_GetSessionPayload(const char* nonce);

/**
 * Core_Poll — session status polling.
 *
 * pid:   Route PID.
 * nonce: Nonce received from Core_Start.
 *
 * JSON response:
 * {
 *   "status": "waiting" | "verified" | "nft_rejected" |
 *             "guest_blocked_no_list" | "offline" |
 *             "locked" | "expired" | "eeprom_missing",
 *
 *   // Only if "verified":
 *   "target_url": "<url>",   → post-auth redirect (if configured)
 *   "role":       "admin" | "guest" | "none",
 *
 *   // Only if "locked":
 *   "remaining":  <seconds>
 * }
 *
 * When status == "verified", the Core has already:
 *   - verified the cryptographic signature
 *   - checked NFT ownership on-chain
 *   - verified the whitelist (if GUEST)
 *   - fired the hardware relay (if configured via Core_RegisterCommand)
 *   - written the access log
 * The server only needs to read "target_url" (if present) and reply.
 */
CoreResult Core_Poll(const char* pid, const char* nonce);

/**
 * Core_Emergency — offline access via local PIN.
 * Available ONLY when the gateway server is unreachable.
 *
 * JSON response:
 * {
 *   "status":     "verified" | "denied" | "locked" | "conflict",
 *   "remaining":  <seconds>    (only if locked),
 *   "target_url": "<url>"      (only if verified + redirect configured)
 * }
 * "conflict" = server is online, emergency access not permitted.
 * The hardware relay is fired internally — not returned in the response.
 */
CoreResult Core_Emergency(const char* nonce, const char* pin, const char* pid);

/** Core_Logout — invalidates the session associated with the UUID cookie. */
void Core_Logout(const char* uuid);


/* =========================================================================
 * SECTION 5 — ADMINISTRATION (requires active ADMIN session)
 * ========================================================================= */

/** Current whitelist. JSON: { "status":"ok"|"forbidden", "list":[...] } */
CoreResult Core_Admin_GetWhitelist(const char* uuid);

/** Pending list (last 10 unauthorized access attempts). */
CoreResult Core_Admin_GetPending(const char* uuid);

/**
 * Promotes an address from the pending list to the whitelist.
 * Immediately written to disk (encrypted vault).
 * JSON: { "status": "ok" | "forbidden" | "not_found" }
 */
CoreResult Core_Admin_ApproveAddress(const char* uuid, const char* address, const char* name = nullptr);

/**
 * Core_Admin_AddAddress — adds an address directly to the whitelist,
 * bypassing the pending list (manual injection from the admin panel).
 *
 * uuid:       admin "session_uuid" cookie.
 * address:    address to add (must start with "ef", min 20 chars).
 * name:       display name, max 20 chars. NULL = no name.
 * valid_from: Unix timestamp start of validity. 0 = no limit.
 * valid_to:   Unix timestamp end of validity.   0 = no limit.
 * schedule:   JSON array of recurring slots, e.g.:
 *               [{"days":[1,3,5],"from":"08:00","to":"18:00"}]
 *             NULL or "" = no time constraint.
 *             days: 0=Sun, 1=Mon, ..., 6=Sat.
 *             All three parameters are independent.
 *
 * JSON: { "status": "ok" | "forbidden" | "already_present" | "invalid_address" }
 */
CoreResult Core_Admin_AddAddress(const char* uuid,
                                  const char* address,
                                  const char* name,
                                  int64_t     valid_from,
                                  int64_t     valid_to,
                                  const char* schedule_json);

/**
 * Core_Admin_GetDeviceLabel — returns the device label.
 * No auth required.
 * JSON: { "status": "ok", "label": "<label>" }
 *       label is "" if not yet set.
 */
CoreResult Core_Admin_GetDeviceLabel(void);

/**
 * Core_Admin_SetDeviceLabel — sets the device label (max 20 chars).
 * Requires active ADMIN session.
 * JSON: { "status": "ok" | "forbidden" }
 */
CoreResult Core_Admin_SetDeviceLabel(const char* uuid, const char* label);

/**
 * Core_Admin_GetNftInfo — checks whether the configured token exists on-chain.
 *
 * No ADMIN session required: useful during initial provisioning when
 * the token has not yet been minted and no admin exists yet.
 *
 * Logic:
 *   1. Checks token existence via MultiTokens::Tokens (state_getStorage)
 *   2. If found, looks up the owner via state_getKeys on
 *      MultiTokens::TokenAccounts[collection][token]
 *
 * JSON response:
 * {
 *   "status":        "ok" | "eeprom_missing" | "rpc_error",
 *   "token_id":      uint32,
 *   "collection_id": uint32,
 *   "minted":        bool,
 *   "owner_address": "<ss58>"   (only if minted == true)
 * }
 */
CoreResult Core_Admin_GetNftInfo(void);

/**
 * Removes an address from the whitelist.
 * Invalidates the address's active sessions. Immediately written to disk.
 * JSON: { "status": "ok" | "forbidden" | "not_found" | "cannot_remove_self" }
 */
CoreResult Core_Admin_RemoveAddress(const char* uuid, const char* address);

/**
 * Updates the emergency PIN (minimum 4 digits).
 * The PIN is hashed with the serial number before storage.
 * JSON: { "status": "ok" | "forbidden" | "pin_too_short" }
 */
CoreResult Core_Admin_UpdatePin(const char* uuid, const char* new_pin);

/**
 * Core_Admin_GetSecurityStatus — system security overview.
 *
 * JSON response:
 * {
 *   "status":               "ok" | "forbidden",
 *   "is_custom_pin":        bool,    — true if PIN was changed from factory default
 *   "server_online":        bool,    — gateway reachability
 *   "token_id":             uint32,  — current NFT token ID (0 = EEPROM missing)
 *   "collection_id":        uint32,  — configured collection
 *   "serial_number":        string,  — device serial
 *   "failed_pin_attempts":  int,     — consecutive wrong PIN attempts
 *   "admin_ownership_lost": bool     — the wallet that signed the whitelist
 *                                      no longer owns the NFT. If true and
 *                                      the admin did not transfer the token,
 *                                      the device may have been compromised.
 * }
 */
CoreResult Core_Admin_GetSecurityStatus(const char* uuid);

/**
 * Core_Admin_SaveWhitelist — starts a signing session (ocu:10) for the
 * provisional whitelist. The admin wallet must scan the QR and sign with
 * sr25519 to activate the changes. Poll /api/hw/status with the returned
 * nonce to track progress. On success the provisional list becomes active
 * and is persisted in the encrypted vault.
 *
 * pid: route PID used for the signing poll (typically "whitelist").
 *
 * JSON response:
 * {
 *   "status":       "online" | "offline" | "eeprom_missing",
 *   "sid":          "<ocu:10...>",
 *   "nonce":        "<geometric_nonce>",
 *   "session_uuid": "<uuid>"
 * }
 */
CoreResult Core_Admin_SaveWhitelist(const char* uuid, const char* pid);

/**
 * Core_Admin_IsWhitelistDirty — checks whether the provisional whitelist
 * differs from the active (signed) version. Used by the frontend to show
 * the "Sign & Save" / "Discard" buttons.
 *
 * JSON response:
 * {
 *   "dirty": bool   — true if there are unsaved changes
 * }
 */
CoreResult Core_Admin_IsWhitelistDirty(const char* uuid);

/**
 * Core_Admin_DiscardChanges — reverts the provisional whitelist to the
 * last signed and activated version. Undoes all staged add/remove/route
 * changes that have not yet been signed.
 *
 * JSON response:
 * {
 *   "status": "ok" | "forbidden"
 * }
 */
CoreResult Core_Admin_DiscardChanges(const char* uuid);

/**
 * Last N lines of the access log. 0 = all lines.
 * JSON: { "status": "ok" | "forbidden", "lines": ["...", ...] }
 */
CoreResult Core_Admin_GetLogs(const char* uuid, int max_lines);

/**
 * Reboots the system (responds before rebooting, async 500ms delay).
 * JSON: { "status": "ok" | "forbidden" }
 */
CoreResult Core_Admin_Reboot(const char* uuid);


/* =========================================================================
 * SECTION 6 — SESSION AUTHORIZATION (for server SecureApi)
 * ========================================================================= */

/**
 * Core_GetRole — returns the role associated with a UUID cookie.
 *
 * uuid: value of the browser "session_uuid" cookie.
 *
 * Returns:
 *   CORE_ROLE_ADMIN  → active and valid admin session
 *   CORE_ROLE_GUEST  → active and valid guest session
 *   CORE_ROLE_NONE   → session absent, expired or not authenticated
 *
 * Typical use in SecureApi (protected route wrapper):
 *   CoreRole role = Core_GetRole(uuid);
 *   if (role < required_role) { res.status = 403; return; }
 *
 * Replaces direct access to sessionMgr.resolveRoleFromUUID(),
 * removing any server dependency on Core internals.
 */
CoreRole Core_GetRole(const char* uuid);

/**
 * Core_CheckAccess — checks whether a UUID has access to a route.
 *
 * uuid: value of the "session_uuid" cookie.
 * pid:  PID of the route to check.
 *
 * Returns true if the session role satisfies the route requirement:
 *   ADMIN required → ADMIN sessions only
 *   GUEST required → GUEST or ADMIN sessions
 *   NONE  required → always true
 *
 * More compact alternative to Core_GetRole() for binary checks.
 * Returns false if the session does not exist or the route is not registered.
 */
bool Core_CheckAccess(const char* uuid, const char* pid);

/**
 * Core_GetAddress — returns the wallet address associated with a UUID session.
 * Returns empty string if the session does not exist or is not authenticated.
 */
const char* Core_GetAddress(const char* uuid);

/* =========================================================================
 * SECTION 7 — DIAGNOSTICS
 * ========================================================================= */

/** SDK version string (static, e.g. "1.2.0"). */
const char* Core_GetVersion(void);

/** true if the OCU gateway is reachable. */
bool Core_IsServerOnline(void);

/** Token ID from EEPROM or override. 0 = not configured. */
uint32_t Core_GetTokenID(void);
/**
 * Core_GetOwnershipStatus — public endpoint, no authentication required.
 * Returns whether the admin wallet that signed the active whitelist
 * still owns the NFT. Intended for display on the login page.
 *
 * JSON response:
 * {
 *   "admin_ownership_lost": bool
 * }
 */
CoreResult Core_GetOwnershipStatus(void);
/**
 * Call from the main loop every ~100ms.
 * Handles the physical factory reset button (≥10s hold = reset + reboot).
 */
void Core_HandleHardwareReset(void);
#ifdef __cplusplus
} /* extern "C" */
#endif