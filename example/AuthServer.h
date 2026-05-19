#pragma once

// ---------------------------------------------------------------------------
//  AuthServer.h — HTTP server wrapper
//
//  Dipendenze: CoreSDK.h + httplib.h + Routes.hpp
//  ZERO dipendenze dagli interni del Core.
//
//  Gestisce:
//    - Avvio server HTTP o HTTPS
//    - Thread di pulizia sessioni scadute (via Core_CleanExpired)
//    - Thread di monitoring hardware reset (via Core_HandleHardwareReset)
// ---------------------------------------------------------------------------

#include "httplib.h"
#include "CoreSDK.h"
#include "Routes.hpp"
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include <iostream>
#include <vector>

class AuthServer {
public:
    struct Config {
        int         port      = 80;
        bool        use_ssl   = false;
        std::string cert_path = "./certs/server.crt";
        std::string key_path  = "./certs/server.key";
        std::vector<ExtraRouteEntry> extraRoutes;
    };

    explicit AuthServer(const Config& config)
        : _config(config), _running(false) {}

    ~AuthServer() { stop(); }

    void stop() {
        _running = false;
        if (_cleanupThread.joinable())   _cleanupThread.join();
        if (_hwResetThread.joinable())   _hwResetThread.join();
    }

    void start() {
        _running = true;

        // Thread di pulizia sessioni scadute — ogni 10 minuti
        // Usa Core_CleanExpired() invece di sessionMgr.cleanExpired() direttamente
        _cleanupThread = std::thread([this]() {
            while (_running) {
                for (int i = 0; i < 600 && _running; ++i)
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                if (_running)
                    Core_CleanExpired();
            }
        });

        // Thread di monitoring hardware reset — ogni 100ms
        _hwResetThread = std::thread([this]() {
            while (_running) {
                Core_HandleHardwareReset();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (_config.use_ssl) {
            auto ssl_svr = std::make_unique<httplib::SSLServer>(
                _config.cert_path.c_str(),
                _config.key_path.c_str()
            );
            if (ssl_svr->is_valid()) {
                run_instance(*ssl_svr);
            } else {
                std::cerr << "[SERVER] ERRORE: certificati SSL non validi.\n";
                stop();
            }
        } else {
            httplib::Server svr;
            run_instance(svr);
        }
#else
        if (_config.use_ssl) {
            std::cerr << "[SERVER] ATTENZIONE: SSL richiesto ma CPPHTTPLIB_OPENSSL_SUPPORT"
                         " non è definito. Avvio in HTTP.\n";
        }
        httplib::Server svr;
        run_instance(svr);
#endif
    }

private:
    Config            _config;
    std::atomic<bool> _running;
    std::thread       _cleanupThread;
    std::thread       _hwResetThread;

    void run_instance(httplib::Server& svr) {
        SetupRoutes(svr, _config.extraRoutes);

        std::cout << "[SERVER] Running on port " << _config.port
                  << (_config.use_ssl ? " (HTTPS)" : " (HTTP)") << "\n";

        if (!svr.listen("0.0.0.0", _config.port)) {
            std::cerr << "[SERVER] ERRORE: impossibile bindare la porta "
                      << _config.port << "\n";
            stop();
        }
    }
};