#ifndef QR_GENERATOR_H
#define QR_GENERATOR_H

#include <string>
#include <vector>
#include <sstream>
#include "opqr/opqr.hpp"

namespace QrGenerator {

    // Helper minimale per convertire i byte in Base64
    // Ci serve perché l'HTML vuole "data:image/png;base64,..."
    inline std::string base64_encode(const std::string& in) {
        static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        int val = 0, valb = -6;
        for (unsigned char c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(lookup[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    inline std::string Generate(const std::string& text) {
        try {
            opqr::QR qr(text);
            auto pic = qr.generate();

            std::stringstream ss;
            // Scriviamo i byte del PNG nello stream
            pic.paint(opqr::pic::Format::PNG, ss, 300, 300);

            return ss.str(); // Restituisce i byte binari del PNG
        }
        catch (...) {
            return "";
        }
    }
}

#endif