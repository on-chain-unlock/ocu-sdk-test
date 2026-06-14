#pragma once

// =============================================================================
//  Routes.hpp — HTTP API layer  (On-Chain Unlock v1.3.0)
//
//  Dependencies: CoreSDK.h, httplib.h, json.hpp, QrGenerator.h
//  ZERO access to Core internals — only the public SDK API is used.
//
//  WHAT THIS FILE DOES:
//    - Exposes all SDK endpoints: /api/hw/*, /api/admin/*, /api/logout
//    - Provides SecureApi() to guard routes by minimum role
//    - Registers HTML page handlers injected by main.cpp (ExtraRouteEntry)
//
//  WHAT THIS FILE DOES NOT DO:
//    - Does not generate HTML (templates live in login.h / whitelist.h)
//    - Does not access Core internals (.so boundary is respected)
//    - Does not store any state (stateless HTTP layer)
//
//  OPEN vs PROTECTED ENDPOINTS:
//    Most /api/admin/* endpoints require an active ADMIN session (uuid cookie).
//    Intentionally open (no auth required):
//      GET  /api/admin/nft/info      — token exists + owner (pre-provisioning)
//      GET  /api/admin/device/label  — device label for wallet SID display
//
//  ROUTE OPEN/CLOSE:
//    POST /api/admin/route/open toggles a GUEST route to NONE at runtime.
//    Requires allow_open=true in Core_RegisterRoute. Always requires a
//    valid_until timestamp — routes cannot be left open indefinitely.
//
//  WHITELIST SIGNING (v1.3.0):
//    POST /api/admin/whitelist/save  — avvia sessione ocu:10 (firma admin)
//    GET  /api/admin/whitelist/dirty — provvisoria != attiva?
//    add/remove modificano la PROVVISORIA; save la firma e la attiva.
//
//  SID FLOW:
//    GET  /api/hw/start  → Core_Start()  → returns SID + nonce
//    GET  /api/hw/status → Core_Poll()   → verifies signature + NFT, fires relay
//    The SID is used by the caller to build QR / NFC / BLE payload.
// =============================================================================

#include "httplib.h"
#include "json.hpp"
#include "CoreSDK.h"
#include "QrGenerator.h"
#include <functional>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
//  ExtraRouteEntry
//
//  Used by main.cpp to inject HTML page handlers into the server.
//  Declare one entry per page — SDK routes are registered by SetupRoutes().
//
//  Fields:
//    method   — "GET" or "POST"
//    path     — e.g. "/relay", "/accesscontrol", "/setting"
//    minRole  — minimum role: CORE_ROLE_NONE / GUEST / ADMIN
//    handler  — function that writes the HTTP response
// -----------------------------------------------------------------------------
struct ExtraRouteEntry {
    std::string method;
    std::string path;
    CoreRole    minRole;
    std::function<void(const httplib::Request&, httplib::Response&)> handler;
};

// -----------------------------------------------------------------------------
//  getCookieValue — extracts a cookie value from the Cookie header
// -----------------------------------------------------------------------------
inline std::string getCookieValue(const std::string& header, const std::string& name) {
    if (header.empty()) return "";
    std::string target = name + "=";
    size_t pos = 0;
    while ((pos = header.find(target, pos)) != std::string::npos) {
        if (pos == 0 || header[pos-1] == ' ' || header[pos-1] == ';') {
            size_t start = pos + target.size();
            size_t end   = header.find(';', start);
            return (end == std::string::npos)
                ? header.substr(start)
                : header.substr(start, end - start);
        }
        pos += target.size();
    }
    return "";
}

// -----------------------------------------------------------------------------
//  SecureApi — registers an HTTP route protected by minimum role
//
//  minRole:
//    CORE_ROLE_NONE  → open to everyone (Identification)
//    CORE_ROLE_GUEST → requires a verified GUEST or ADMIN session
//    CORE_ROLE_ADMIN → requires a verified ADMIN session
// -----------------------------------------------------------------------------
inline void SecureApi(httplib::Server& svr,
                      const std::string& path,
                      const std::string& method,
                      CoreRole minRole,
                      std::function<void(const httplib::Request&, httplib::Response&)> handler)
{
    auto wrapper = [minRole, handler](const httplib::Request& req, httplib::Response& res) {
        std::string origin = req.get_header_value("Origin");
        res.set_header("Access-Control-Allow-Origin",      origin.empty() ? "*" : origin);
        res.set_header("Access-Control-Allow-Credentials", "true");
        res.set_header("Access-Control-Allow-Methods",     "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers",     "Content-Type");

        std::string uuid     = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
        CoreRole    userRole = Core_GetRole(uuid.c_str());

        bool ok =
            (minRole == CORE_ROLE_NONE) ||
            (minRole == CORE_ROLE_GUEST && userRole >= CORE_ROLE_GUEST) ||
            (minRole == CORE_ROLE_ADMIN && userRole == CORE_ROLE_ADMIN);

        if (!ok) {
            res.status = 403;
            res.set_content(R"({"status":"forbidden"})", "application/json");
            return;
        }
        handler(req, res);
    };

    if      (method == "GET")  svr.Get (path, wrapper);
    else if (method == "POST") svr.Post(path, wrapper);
}

// -----------------------------------------------------------------------------
//  SetupRoutes — registers all routes on the httplib server
//
//  Fixed registration order:
//    1. CORS preflight
//    2. SDK hardware API  (/api/hw/*)
//    3. SDK admin API     (/api/admin/*)
//    4. /api/logout
//    5. HTML pages        (extraRoutes from main.cpp)
// -----------------------------------------------------------------------------
inline void SetupRoutes(httplib::Server& svr,
                        const std::vector<ExtraRouteEntry>& extraRoutes = {})
{
    // =========================================================================
    // 1. CORS preflight
    // =========================================================================
    svr.Options(R"(.*)", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin",      req.get_header_value("Origin"));
        res.set_header("Access-Control-Allow-Methods",     "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers",     "Content-Type");
        res.set_header("Access-Control-Allow-Credentials", "true");
        res.status = 204;
    });

    // =========================================================================
    // 2. API SDK HARDWARE
    // =========================================================================

    // GET /api/hw/start/<pid>
    // Starts an authentication session for <pid>.
    // Risposta: { sid, nonce, session_uuid, status, admin_locked }
    // The session_uuid cookie is set automatically.
    svr.Get(R"(/api/hw/start/(\w+))", [](const httplib::Request& req, httplib::Response& res) {
        std::string pid  = req.matches[1];
        std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");

        CoreResult r = Core_Start(pid.c_str(), uuid.c_str());

        std::string newUuid;
        try {
            auto j = nlohmann::json::parse(r.json_payload);
            newUuid = j.value("session_uuid", "");
        } catch (...) {}


        if (!newUuid.empty())
            res.set_header("Set-Cookie",
                "session_uuid=" + newUuid +
                "; Path=/; HttpOnly; SameSite=Lax; Max-Age=3600");

        res.status = r.http_code;
        res.set_content(r.json_payload, "application/json");
    });

    // GET /api/hw/status?pid=<pid>&n=<nonce>
    // Session status polling. Core verifies signature + NFT/whitelist
    // and fires the relay internally if verified.
    // Risposta: { status, target_url, role, remaining }
    svr.Get("/api/hw/status", [](const httplib::Request& req, httplib::Response& res) {
        std::string pid   = req.get_param_value("pid");
        std::string nonce = req.get_param_value("n");
        if (nonce.empty()) {
            res.status = 200;
            res.set_content(R"({"status":"expired"})", "application/json");
            return;
        }
        CoreResult r = Core_Poll(pid.c_str(), nonce.c_str());
        res.status = r.http_code;
        res.set_content(r.json_payload, "application/json");
    });

    // GET /api/hw/sid?n=<nonce>
    // Retrieves SID and nonce of an active session.
    // Used for QR refresh, new NFC write, etc.
    svr.Get("/api/hw/sid", [](const httplib::Request& req, httplib::Response& res) {
        std::string nonce = req.get_param_value("n");
        CoreResult r = Core_GetSessionPayload(nonce.c_str());
        res.status = r.http_code;
        res.set_content(r.json_payload, "application/json");
    });

    // GET /api/hw/qr?n=<nonce>
    // Generates a QR code image from the session SID (PNG).
    svr.Get("/api/hw/qr", [](const httplib::Request& req, httplib::Response& res) {
        std::string nonce = req.get_param_value("n");
        if (nonce.empty()) { res.status = 400; return; }

        CoreResult p = Core_GetSessionPayload(nonce.c_str());
        if (p.http_code != 200) { res.status = 404; return; }

        auto j = nlohmann::json::parse(p.json_payload);
        std::string sid = j.value("sid", "");
        if (sid.empty() || sid == "00000000") { res.status = 404; return; }

        std::string qr = QrGenerator::Generate(sid);
        if (qr.empty()) { res.status = 500; return; }
        res.set_content(qr, "image/png");
    });

    // POST /api/hw/emergency-unlock
    // Offline access via local PIN. Only when gateway is unreachable.
    // Body JSON: { "pin": "1234", "pid": "relay" }
    // Nonce as query param: ?n=<nonce>
    svr.Post("/api/hw/emergency-unlock", [](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string nonce = req.get_param_value("n");
            auto j = nlohmann::json::parse(req.body);
            std::string pid = j.value("pid",   "");
            std::string pin = j.value("pin",   "");
            if (nonce.empty()) nonce = j.value("nonce", "");
            if (nonce.empty()) {
                res.status = 401;
                res.set_content(R"({"status":"expired"})", "application/json");
                return;
            }
            CoreResult r = Core_Emergency(nonce.c_str(), pin.c_str(), pid.c_str());
            res.status = r.http_code;
            res.set_content(r.json_payload, "application/json");
        } catch (...) { res.status = 400; }
    });

    // =========================================================================
    // 3. API SDK ADMIN  (tutte protette da CORE_ROLE_ADMIN)
    // =========================================================================

    // GET /api/admin/whitelist/pending
    // Last 10 addresses that attempted access without authorization.
    SecureApi(svr, "/api/admin/whitelist/pending", "GET", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            CoreResult r = Core_Admin_GetPending(uuid.c_str());
            auto j = nlohmann::json::parse(r.json_payload);
            res.status = r.http_code;
            res.set_content(j.value("list", nlohmann::json::array()).dump(), "application/json");
        });

    // GET /api/admin/whitelist/active
    // Whitelist mostrata in UI: provvisoria se ci sono modifiche non firmate,
    // altrimenti attiva. Include vincoli temporali per ogni entry.
    // Risposta: array di { address, valid_from, valid_to, schedule }
    SecureApi(svr, "/api/admin/whitelist/active", "GET", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            CoreResult r = Core_Admin_GetWhitelist(uuid.c_str());
            auto j = nlohmann::json::parse(r.json_payload);
            res.status = r.http_code;
            res.set_content(j.value("list", nlohmann::json::array()).dump(), "application/json");
        });

    // POST /api/admin/whitelist/add
    // Aggiunge un indirizzo alla PROVVISORIA (da pending o diretto).
    // NB: la modifica NON e' attiva finche' l'admin non firma via /save.
    // Body JSON: {
    //   "address":    "ef...",          (required)
    //   "valid_from": 1700000000,       (optional, Unix timestamp, 0 = no limit)
    //   "valid_to":   1800000000,       (optional, Unix timestamp, 0 = no limit)
    //   "schedule": [                   (optional, empty = always)
    //     { "days":[1,3,5], "from":"08:00", "to":"18:00" }
    //   ]
    // }
    SecureApi(svr, "/api/admin/whitelist/add", "POST", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            try {
                auto j             = nlohmann::json::parse(req.body);
                std::string addr   = j.value("address",    "");
                std::string name   = j.value("name",       "");
                int64_t valid_from = j.value("valid_from", (int64_t)0);
                int64_t valid_to   = j.value("valid_to",   (int64_t)0);
                std::string sched  = j.contains("schedule") ? j["schedule"].dump() : "";

                // First checks pending list, then adds directly if not found
                CoreResult r = Core_Admin_ApproveAddress(uuid.c_str(), addr.c_str(),
                                                          name.empty() ? nullptr : name.c_str());
                if (r.http_code == 404) {
                    r = Core_Admin_AddAddress(uuid.c_str(), addr.c_str(),
                                              name.empty() ? nullptr : name.c_str(),
                                              valid_from, valid_to,
                                              sched.empty() ? nullptr : sched.c_str());
                }
                res.status = r.http_code;
                res.set_content(r.json_payload, "application/json");
            } catch (...) { res.status = 400; }
        });

    // POST /api/admin/whitelist/remove
    // Rimuove un indirizzo dalla PROVVISORIA e invalida le sue sessioni attive.
    // NB: la modifica NON e' attiva finche' l'admin non firma via /save.
    // Body JSON: { "address": "ef..." }
    SecureApi(svr, "/api/admin/whitelist/remove", "POST", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            try {
                auto j = nlohmann::json::parse(req.body);
                std::string addr = j.value("address", "");
                CoreResult r = Core_Admin_RemoveAddress(uuid.c_str(), addr.c_str());
                res.status = r.http_code;
                res.set_content(r.json_payload, "application/json");
            } catch (...) { res.status = 400; }
        });

    // POST /api/admin/whitelist/save
    // Avvia una sessione ocu:10 per far firmare la whitelist provvisoria
    // all'admin (wallet sr25519). Il frontend mostra il QR del SID ritornato,
    // l'admin firma, il poll su /api/hw/status completa il commit.
    // Risposta: { sid, nonce, session_uuid, status }
    SecureApi(svr, "/api/admin/whitelist/save", "POST", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            CoreResult r = Core_Admin_SaveWhitelist(uuid.c_str(), "whitelist");
            res.status = r.http_code;
            res.set_content(r.json_payload, "application/json");
        });

    // GET /api/admin/whitelist/dirty
    // Ritorna { dirty: true/false } — la provvisoria differisce dall'attiva?
    // Usato dalla UI per ricordare all'admin di firmare e salvare.
    SecureApi(svr, "/api/admin/whitelist/dirty", "GET", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            CoreResult r = Core_Admin_IsWhitelistDirty(uuid.c_str());
            res.status = r.http_code;
            res.set_content(r.json_payload, "application/json");
        });
    // POST /api/admin/whitelist/discard
    // Reverts provisional whitelist to the active (signed) version.
    // Undoes all staged changes.
    SecureApi(svr, "/api/admin/whitelist/discard", "POST", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            CoreResult r = Core_Admin_DiscardChanges(uuid.c_str());
            res.status = r.http_code;
            res.set_content(r.json_payload, "application/json");
    });
    // GET /api/admin/security/status
    // System status: custom PIN, server online, token ID, failed attempts...
    SecureApi(svr, "/api/admin/security/status", "GET", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            CoreResult r = Core_Admin_GetSecurityStatus(uuid.c_str());
            res.status = r.http_code;
            res.set_content(r.json_payload, "application/json");
        });

    // POST /api/admin/security/update-pin
    // Updates the emergency PIN (min 4 digits, hashed with serial number).
    // Body JSON: { "pin": "1234" }
    SecureApi(svr, "/api/admin/security/update-pin", "POST", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            try {
                auto j = nlohmann::json::parse(req.body);
                std::string pin = j.value("pin", "");
                CoreResult r = Core_Admin_UpdatePin(uuid.c_str(), pin.c_str());
                res.status = r.http_code;
                res.set_content(r.json_payload, "application/json");
            } catch (...) { res.status = 400; }
        });

    // GET /api/admin/logs?n=<righe>
    // Last N lines of the access log (0 = all).
    SecureApi(svr, "/api/admin/logs", "GET", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            int lines = 100;
            if (req.has_param("n")) {
                try { lines = std::stoi(req.get_param_value("n")); } catch (...) {}
            }
            CoreResult r = Core_Admin_GetLogs(uuid.c_str(), lines);
            res.status = r.http_code;
            res.set_content(r.json_payload, "application/json");
        });

    // POST /api/admin/reboot
    // Reboots the system (responds before rebooting, async 500ms).
    SecureApi(svr, "/api/admin/reboot", "POST", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            CoreResult r = Core_Admin_Reboot(uuid.c_str());
            res.status = r.http_code;
            res.set_content(r.json_payload, "application/json");
        });

    // POST /api/admin/route/mode
    // Changes a route's access level at runtime (RAM only, no persistence).
    // Route must have allow_open=true in Core_RegisterRoute.
    // Body: { "pid": "relay", "mode": 0|1|2, "valid_until": 1700000000 }
    //   mode 0 = NORMAL  → restore to GUEST (whitelist)
    //   mode 1 = OPEN    → GUEST becomes NONE (anyone enters, wallet logged)
    //   mode 2 = LOCKED  → GUEST becomes ADMIN (NFT owner only — emergency lockdown)
    //   valid_until: Unix timestamp for automatic revert to NORMAL (0 = manual only)
    SecureApi(svr, "/api/admin/route/mode", "POST", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            try {
                auto j          = nlohmann::json::parse(req.body);
                std::string pid = j.value("pid",         "");
                int         m   = j.value("mode",        0);
                int64_t     vu  = j.value("valid_until", (int64_t)0);
                CoreRouteMode mode = (m == 1) ? CORE_ROUTE_OPEN :
                                     (m == 2) ? CORE_ROUTE_LOCKED : CORE_ROUTE_NORMAL;
                CoreResult r = Core_SetRouteMode(uuid.c_str(), pid.c_str(), mode, vu);
                res.status = r.http_code;
                res.set_content(r.json_payload, "application/json");
            } catch (...) { res.status = 400; }
        });

    // GET /api/admin/routes — registered routes with allow_open flag and current state
    SecureApi(svr, "/api/admin/routes", "GET", CORE_ROLE_ADMIN,
        [](const httplib::Request&, httplib::Response& res) {
            CoreResult r = Core_GetRoutes();
            res.status = r.http_code;
            res.set_content(r.json_payload, "application/json");
        });

    // GET /api/admin/device/label — device label (open, no auth required)
    svr.Get("/api/admin/device/label", [](const httplib::Request&, httplib::Response& res) {
        CoreResult r = Core_Admin_GetDeviceLabel();
        res.status = r.http_code;
        res.set_content(r.json_payload, "application/json");
    });

    // POST /api/admin/device/label — imposta label (richiede admin)
    // Body: { "label": "BUILDING-A ENTRANCE" }
    SecureApi(svr, "/api/admin/device/label", "POST", CORE_ROLE_ADMIN,
        [](const httplib::Request& req, httplib::Response& res) {
            std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
            try {
                auto j = nlohmann::json::parse(req.body);
                std::string label = j.value("label", "");
                CoreResult r = Core_Admin_SetDeviceLabel(uuid.c_str(), label.c_str());
                res.status = r.http_code;
                res.set_content(r.json_payload, "application/json");
            } catch (...) { res.status = 400; }
        });

    // GET /api/admin/nft/info
    // Open endpoint — no admin session required.
    // Useful during initial provisioning before the NFT is minted.
    svr.Get("/api/admin/nft/info", [](const httplib::Request&, httplib::Response& res) {
        CoreResult r = Core_Admin_GetNftInfo();
        res.status = r.http_code;
        res.set_content(r.json_payload, "application/json");
    });
    // GET /api/ownership
    // Public endpoint — no authentication required.
    // Returns whether the admin that signed the whitelist still owns the NFT.
    // Used by the login page to show an ownership warning banner.
    svr.Get("/api/ownership", [](const httplib::Request&, httplib::Response& res) {
        CoreResult r = Core_GetOwnershipStatus();
        res.status = r.http_code;
        res.set_content(r.json_payload, "application/json");
    });
    // =========================================================================
    // 4. LOGOUT
    // =========================================================================
    svr.Post("/api/logout", [](const httplib::Request& req, httplib::Response& res) {
        std::string uuid = getCookieValue(req.get_header_value("Cookie"), "session_uuid");
        Core_Logout(uuid.c_str());
        res.set_header("Set-Cookie",
            "session_uuid=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0");
        res.set_content(R"({"status":"logged_out"})", "application/json");
    });

    // =========================================================================
    // 5. PAGINE HTML — iniettate da main.cpp via extraRoutes
    //
    // Registered last: must not overlap with the SDK API routes above.
    // The integrator chooses the template and content for each page.
    // =========================================================================
    for (const auto& route : extraRoutes) {
        SecureApi(svr, route.path, route.method, route.minRole, route.handler);
    }
}
