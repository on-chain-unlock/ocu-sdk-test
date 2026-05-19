// =============================================================================
//  main_with_sdk.cpp — Esempio di integrazione CoreSDK
//
//  This is the only file the integrator needs to write.
//  Everything else (security, NFT, whitelist, hardware) lives inside CoreSDK.so.
//
//  To add a new door/route:
//    1. Core_RegisterRoute(pid, role, title, redirect)
//    2. Core_RegisterCommand(pid, gpio_pin, duration_ms)  ← only if hardware present
//    3. Add the HTML page to extraRoutes
//
//  Available roles:
//    CORE_ROLE_ADMIN → NFT token owner only
//    CORE_ROLE_GUEST → addresses in the whitelist (or admin)
//    CORE_ROLE_NONE  → anyone who signs enters, wallet logged (open access)
// =============================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NOMINMAX

#include <iostream>
#include "CoreSDK.h"
#include "httplib.h"
#include "json.hpp"
#include "login.h"       // getTemplate("guest"|"admin"), replacePlaceholder()
#include "whitelist.h"   // getAdminTemplate() — access management panel
#include "Routes.hpp"    // SetupRoutes(), SecureApi(), ExtraRouteEntry
#include "AuthServer.h"  // AuthServer — avvia httplib + thread di manutenzione

#ifdef _WIN32
#include <winsock2.h>
#endif

// =============================================================================
//  HARDWARE CALLBACKS (Linux/ARM64)
//
//  Implement these functions to match your hardware wiring.
//
//  GPIO: adapt pin numbers and sysfs paths to your board.
//  EEPROM: adapt I2C bus, address and register to your hardware.
//
//  ⚠ WARNING: hw_gpio_write() controls physical relays.
//     Test in a safe environment before connecting real hardware.
//     Wrong pin numbers can damage connected devices.
//
//  On Windows these compile as no-op stubs for development only.
// =============================================================================
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <sys/stat.h>

// Read GPIO pin state via sysfs.
// Returns 1 (HIGH) on error — fail-safe default.
static int hw_gpio_read(int pin) {
    std::string path = "/sys/class/gpio/gpio" + std::to_string(pin) + "/value";
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return 1; // fail-safe: treat as HIGH
    char buf[2] = {};
    read(fd, buf, 1);
    close(fd);
    return (buf[0] == '0') ? 0 : 1;
}

// Write GPIO pin state via sysfs.
// Exports the pin if not already exported.
// ⚠ Adapt pin numbers to your board before use.
static void hw_gpio_write(int pin, int value) {
    std::string gpioPath = "/sys/class/gpio/gpio" + std::to_string(pin);
    struct stat st{};
    if (stat(gpioPath.c_str(), &st) != 0) {
        // Export pin if not already exported
        int fd = open("/sys/class/gpio/export", O_WRONLY);
        if (fd >= 0) { dprintf(fd, "%d", pin); close(fd); }
        usleep(100000); // wait for sysfs to create the entry
        int dfd = open((gpioPath + "/direction").c_str(), O_WRONLY);
        if (dfd >= 0) { write(dfd, "out", 3); close(dfd); }
    }
    int fd = open((gpioPath + "/value").c_str(), O_WRONLY);
    if (fd >= 0) { dprintf(fd, "%d", value); close(fd); }
}

// Read TokenID from EEPROM via I2C.
// Returns 0 if the read fails — Core will deny access.
// ⚠ Adapt i2c_addr and reg to your hardware before use.
static uint32_t hw_eeprom_read(uint8_t i2c_addr, uint8_t reg) {
    int file = open("/dev/i2c-1", O_RDWR);
    if (file < 0) return 0;
    if (ioctl(file, I2C_SLAVE, i2c_addr) < 0) { close(file); return 0; }
    if (write(file, &reg, 1) != 1)             { close(file); return 0; }
    uint32_t val = 0;
    if (read(file, &val, sizeof(val)) < 0)     { close(file); return 0; }
    close(file);
    return val;
}

#else
// Windows stubs — no-op, safe for development only.
// GPIO and EEPROM are not available on Windows.
static int      hw_gpio_read(int)         { return 1; }
static void     hw_gpio_write(int, int v) { printf("[WIN] GPIO -> %d\n", v); }
// EEPROM stub not needed on Windows — see Core_Configure() call below.
#endif


// =============================================================================
//  MAIN
// =============================================================================
int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 1;
#endif

    // =========================================================================
    // STEP 1 — HARDWARE
    //
    // Inject GPIO and EEPROM callbacks into the Core.
    // After this call the Core manages all hardware internally.
    // Never call GPIO or EEPROM directly from the server.
    //
    // ⚠ On Linux: adapt pin numbers in hw_gpio_write() and EEPROM parameters
    //   (i2c_addr, eeprom_token_reg) to match your hardware before deploying.
    // =========================================================================
    CoreHardwareConfig hw = {};
    hw.gpio_read        = hw_gpio_read;
    hw.gpio_write       = hw_gpio_write;
#ifndef _WIN32
    // EEPROM is only available on Linux/ARM64.
    // In CORE_TEST_MODE without EEPROM, token_id is read from test.cfg.
    hw.eeprom_read      = hw_eeprom_read;
    hw.eeprom_i2c_addr  = 0x50;   // ⚠ adapt to your EEPROM I2C address
    hw.eeprom_token_reg = 0x00;   // ⚠ adapt to the register storing the TokenID
    hw.i2c_bus          = "/dev/i2c-1"; // ⚠ adapt to your I2C bus
#endif
    Core_Configure(&hw);

    // =========================================================================
    // STEP 1b — STORAGE OVERRIDES (optional)
    //
    // Override specific storage parameters at runtime.
    // In production these are injected into the binary by inject_config.py.
    // Use this call to override individual values — NULL/zero fields are ignored.
    //
    // Log rotation defaults: 10MB threshold, keep last 2000 lines.
    // Adjust for your deployment — high-traffic installations may need more.
    // =========================================================================
    // CoreStorageConfig storageCfg = {};
    // storageCfg.log_path       = "/data/access_log.txt";
    // storageCfg.log_max_bytes  = 10 * 1024 * 1024;  // 10MB
    // storageCfg.log_keep_lines = 2000;
    // Core_ConfigureStorage(&storageCfg);

    // =========================================================================
    // STEP 2 — CORE INIT
    //
    // Reads configuration (test.cfg in development, binary in production),
    // reads TokenID from EEPROM, loads the encrypted whitelist.
    // Returns false on critical configuration error.
    // =========================================================================
    if (!Core_Init()) {
        std::cerr << "[MAIN] Core init failed. Check test.cfg or config.txt\n";
        return 1;
    }

    // =========================================================================
    // STEP 3 — ROUTE REGISTRY
    //
    // For each physical access point (door, gate, etc.) two calls are needed:
    //
    //   Core_RegisterRoute(pid, role, title, redirect, allow_open)
    //     pid        → unique identifier, becomes the login URL (/<pid>)
    //     role       → who can access (ADMIN / GUEST / NONE)
    //     title      → human-readable name shown on the login page
    //     redirect   → URL after authentication, NULL = no redirect
    //                  (volatile session: relay fires and session closes)
    //     allow_open → if true, route can be toggled GUEST↔NONE at runtime
    //
    //   Core_RegisterCommand(pid, gpio_pin, duration_ms)
    //     Binds the pid to a physical relay. The Core fires it automatically
    //     when access is verified. No further calls needed.
    //     Omit if the route has no hardware (e.g. admin panel).
    //
    // ⚠ GPIO pin numbers below are examples only.
    //   Adapt them to your wiring before deploying on real hardware.
    // =========================================================================

    // --- Main door ---
    // GUEST: whitelist addresses (or admin) enter.
    // Relay on GPIO 18 for 2 seconds.
    // ⚠ Change pin 18 to match your relay wiring.
    Core_RegisterRoute  ("relay",    CORE_ROLE_GUEST, "DOOR-1",         nullptr, true);
    Core_RegisterCommand("relay",    18, 2000); // ⚠ adapt pin and duration

    // --- Admin access panel ---
    // ADMIN: NFT token owner only.
    // No hardware. After login redirects to /accesscontrol.
    Core_RegisterRoute  ("accesses", CORE_ROLE_ADMIN, "ACCESS CONTROL", "/accesscontrol", false);

    // --- Settings panel ---
    // ADMIN: NFT token owner only.
    // No hardware. After login redirects to /setting.
    Core_RegisterRoute  ("admin",    CORE_ROLE_ADMIN, "ADMIN PANEL",    "/setting", false);

    // --- Example: open access ---
    // NONE: anyone who signs enters. Wallet is logged, no whitelist required.
    // Useful for public events, open houses, etc.
    // Uncomment to enable:
    //
    // Core_RegisterRoute  ("entrance", CORE_ROLE_NONE, "OPEN HOUSE", nullptr, false);
    // Core_RegisterCommand("entrance", 20, 1500);  // ⚠ adapt pin
    //
    // To open/close the route at runtime from the admin panel:
    //   POST /api/admin/route/open  { "pid": "relay", "open": true,  "valid_until": <ts> }  ← GUEST→NONE
    //   POST /api/admin/route/open  { "pid": "relay", "open": false, "valid_until": 0    }  ← NONE→GUEST

    // =========================================================================
    // STEP 4 — HTTP SERVER
    // =========================================================================
    AuthServer::Config cfg;
    cfg.port    = 8080;
    cfg.use_ssl = false;
    // cfg.cert_path = "./certs/server.crt";  // uncomment for HTTPS
    // cfg.key_path  = "./certs/server.key";

    cfg.extraRoutes = {

        // -- /relay ----------------------------------------------------------
        {
            "GET", "/relay", CORE_ROLE_NONE,
            [](const httplib::Request&, httplib::Response& res) {
                std::string html = getTemplate("guest");
                replacePlaceholder(html, "{{PAGE_PID}}", "relay");
                replacePlaceholder(html, "{{TITLE}}",    "DOOR-1");
                replacePlaceholder(html, "{{SERVER_STATE}}",
                    Core_IsServerOnline() ? "true" : "false");
                res.set_content(html, "text/html");
            }
        },

        // -- /accesses -------------------------------------------------------
        {
            "GET", "/accesses", CORE_ROLE_NONE,
            [](const httplib::Request&, httplib::Response& res) {
                std::string html = getTemplate("admin");
                replacePlaceholder(html, "{{PAGE_PID}}", "accesses");
                replacePlaceholder(html, "{{TITLE}}",    "ACCESS CONTROL");
                replacePlaceholder(html, "{{SERVER_STATE}}",
                    Core_IsServerOnline() ? "true" : "false");
                res.set_content(html, "text/html");
            }
        },

        // -- /accesscontrol --------------------------------------------------
        {
            "GET", "/accesscontrol", CORE_ROLE_ADMIN,
            [](const httplib::Request&, httplib::Response& res) {
                res.set_content(getAdminTemplate(), "text/html");
            }
        },

        // -- /admin ----------------------------------------------------------
        {
            "GET", "/admin", CORE_ROLE_NONE,
            [](const httplib::Request&, httplib::Response& res) {
                std::string html = getTemplate("admin");
                replacePlaceholder(html, "{{PAGE_PID}}", "admin");
                replacePlaceholder(html, "{{TITLE}}",    "ADMIN PANEL");
                replacePlaceholder(html, "{{SERVER_STATE}}",
                    Core_IsServerOnline() ? "true" : "false");
                res.set_content(html, "text/html");
            }
        },

        // -- /setting --------------------------------------------------------
        {
            "GET", "/setting", CORE_ROLE_ADMIN,
            [](const httplib::Request&, httplib::Response& res) {
                res.set_content(getAdminTemplate(), "text/html");
            }
        },

    };

    // =========================================================================
    // STARTUP
    // =========================================================================
    std::cout << "-------------------------------------\n"
              << "CoreSDK v" << Core_GetVersion() << "\n"
              << "TokenID:  " << Core_GetTokenID()  << "\n"
              << "PORT:     " << cfg.port            << "\n"
              << "SSL:      " << (cfg.use_ssl ? "YES" : "NO") << "\n"
              << "-------------------------------------\n";

    AuthServer server(cfg);
    server.start(); // blocking — process stays here until terminated

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}