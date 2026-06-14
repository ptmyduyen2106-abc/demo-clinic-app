#include "AuthManager.hpp"
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <openssl/hmac.h>
#include <openssl/evp.h>

// ─────────────────────────────────────────────
// AuthManager — implementation
// ─────────────────────────────────────────────

AuthManager::AuthManager(const std::string& supabaseJwtSecret)
    : m_jwtSecret(supabaseJwtSecret) {}

// ── Public: validate from HTTP request ───────
AuthClaims AuthManager::validateRequest(const httplib::Request& req) const {
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) return {};

    const std::string& authHeader = it->second;
    if (authHeader.size() < 8 || authHeader.substr(0, 7) != "Bearer ") return {};

    return validateToken(authHeader.substr(7));
}

// ── Public: validate raw token ────────────────
AuthClaims AuthManager::validateToken(const std::string& token) const {
    AuthClaims claims;
    try {
        if (!verifySignature(token)) return claims;

        json payload = decodePayload(token);

        // Kiểm tra expiry
        if (payload.contains("exp")) {
            auto exp = payload["exp"].get<long long>();
            auto now = std::chrono::system_clock::now();
            auto nowTs = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()).count();
            if (nowTs > exp) return claims;   // token hết hạn
        }

        // Extract sub (user id)
        if (payload.contains("sub"))
            claims.userId = payload["sub"].get<std::string>();

        // Extract email
        if (payload.contains("email"))
            claims.email = payload["email"].get<std::string>();

        // Supabase lưu role trong app_metadata hoặc user_metadata
        if (payload.contains("app_metadata") &&
            payload["app_metadata"].contains("role")) {
            claims.role = roleFromString(
                payload["app_metadata"]["role"].get<std::string>()
            );
        } else if (payload.contains("role")) {
            claims.role = roleFromString(payload["role"].get<std::string>());
        }

        claims.valid = !claims.userId.empty() && claims.role != UserRole::Unknown;
    } catch (const std::exception& e) {
        std::cerr << "[AuthManager] Token parse error: " << e.what() << "\n";
    }
    return claims;
}

// ── Middleware: require any valid auth ────────
AuthClaims AuthManager::requireAuth(
    const httplib::Request& req,
    httplib::Response& res
) const {
    auto claims = validateRequest(req);
    if (!claims.valid) {
        res.status = 401;
        res.set_content(R"({"success":false,"message":"Unauthorized","statusCode":401})",
                        "application/json");
    }
    return claims;
}

// ── Middleware: require specific role ─────────
AuthClaims AuthManager::requireRole(
    const httplib::Request& req,
    httplib::Response& res,
    std::function<bool(const AuthClaims&)> roleCheck
) const {
    auto claims = requireAuth(req, res);
    if (!claims.valid) return claims;

    if (!roleCheck(claims)) {
        claims.valid = false;
        res.status = 403;
        res.set_content(R"({"success":false,"message":"Forbidden","statusCode":403})",
                        "application/json");
    }
    return claims;
}

// ── Static helpers ────────────────────────────
UserRole AuthManager::roleFromString(const std::string& s) {
    if (s == "patient")    return UserRole::Patient;
    if (s == "doctor")     return UserRole::Doctor;
    if (s == "pharma")     return UserRole::Pharmacist;
    if (s == "admin")      return UserRole::Admin;
    return UserRole::Unknown;
}

std::string AuthManager::roleToString(UserRole r) {
    switch (r) {
        case UserRole::Patient:    return "patient";
        case UserRole::Doctor:     return "doctor";
        case UserRole::Pharmacist: return "pharma";
        case UserRole::Admin:      return "admin";
        default:                   return "unknown";
    }
}

// ── Private: JWT verify signature ─────────────
bool AuthManager::verifySignature(const std::string& token) const {
    // JWT format: header.payload.signature
    auto dot1 = token.find('.');
    auto dot2 = token.find('.', dot1 + 1);
    if (dot1 == std::string::npos || dot2 == std::string::npos) return false;

    std::string signingInput = token.substr(0, dot2);
    std::string sigB64       = token.substr(dot2 + 1);

    // HMAC-SHA256
    unsigned char hmacResult[EVP_MAX_MD_SIZE];
    unsigned int  hmacLen = 0;

    HMAC(EVP_sha256(),
         m_jwtSecret.data(), (int)m_jwtSecret.size(),
         (const unsigned char*)signingInput.data(), signingInput.size(),
         hmacResult, &hmacLen);

    // base64url encode computed signature
    auto b64urlEncode = [](const unsigned char* data, unsigned int len) {
        static const char* chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        for (unsigned int i = 0; i < len; i += 3) {
            unsigned int group = ((unsigned int)data[i] << 16)
                | (i+1 < len ? (unsigned int)data[i+1] << 8 : 0)
                | (i+2 < len ? (unsigned int)data[i+2]      : 0);
            result += chars[(group >> 18) & 0x3F];
            result += chars[(group >> 12) & 0x3F];
            if (i+1 < len) result += chars[(group >> 6) & 0x3F];
            if (i+2 < len) result += chars[(group)      & 0x3F];
        }
        // Chuyển thành base64url (không padding)
        for (auto& c : result) {
            if (c == '+') c = '-';
            if (c == '/') c = '_';
        }
        return result;
    };

    std::string computed = b64urlEncode(hmacResult, hmacLen);
    return computed == sigB64;
}

// ── Private: decode JWT payload ───────────────
json AuthManager::decodePayload(const std::string& token) const {
    auto dot1 = token.find('.');
    auto dot2 = token.find('.', dot1 + 1);
    std::string payloadB64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string payloadJson = base64UrlDecode(payloadB64);
    return json::parse(payloadJson);
}

std::string AuthManager::base64UrlDecode(const std::string& in) const {
    std::string s = in;
    for (auto& c : s) {
        if (c == '-') c = '+';
        if (c == '_') c = '/';
    }
    while (s.size() % 4 != 0) s += '=';

    static const std::string b64chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[(unsigned char)b64chars[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : s) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back((char)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}
