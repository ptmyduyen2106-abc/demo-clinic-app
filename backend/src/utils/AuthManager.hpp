#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include "../libs/json.hpp"
#include "../libs/httplib.h"

using json = nlohmann::json;

// ─────────────────────────────────────────────
// UserRole — khớp với DB check constraint
// ─────────────────────────────────────────────
enum class UserRole {
    Patient,
    Doctor,
    Pharmacist,
    Admin,
    Unknown
};

// ─────────────────────────────────────────────
// AuthClaims — thông tin extract từ JWT
// ─────────────────────────────────────────────
struct AuthClaims {
    std::string userId;
    std::string email;
    UserRole    role    = UserRole::Unknown;
    bool        valid   = false;

    bool isPatient()    const { return role == UserRole::Patient; }
    bool isDoctor()     const { return role == UserRole::Doctor; }
    bool isPharmacist() const { return role == UserRole::Pharmacist; }
    bool isAdmin()      const { return role == UserRole::Admin; }

    // Admin kế thừa toàn bộ quyền
    bool canAccessDoctor()    const { return isDoctor()     || isAdmin(); }
    bool canAccessPharmacy()  const { return isPharmacist() || isAdmin(); }
    bool canAccessFinance()   const { return isAdmin(); }
    bool canAccessOwnData()   const { return isPatient()    || isAdmin(); }
};

// ─────────────────────────────────────────────
// AuthManager — xác thực token Supabase JWT
// ─────────────────────────────────────────────
class AuthManager {
public:
    explicit AuthManager(const std::string& supabaseJwtSecret);

    // Extract và validate JWT từ Authorization header
    // Bearer <token>
    AuthClaims validateRequest(const httplib::Request& req) const;

    // Validate raw JWT string
    AuthClaims validateToken(const std::string& token) const;

    // Middleware helpers — trả về claims nếu hợp lệ
    // Nếu không, tự ghi response 401/403 và trả về claims.valid=false
    AuthClaims requireAuth(
        const httplib::Request& req,
        httplib::Response& res
    ) const;

    AuthClaims requireRole(
        const httplib::Request& req,
        httplib::Response& res,
        std::function<bool(const AuthClaims&)> roleCheck
    ) const;

    // Helper tĩnh
    static UserRole   roleFromString(const std::string& s);
    static std::string roleToString(UserRole r);

private:
    std::string m_jwtSecret;

    // Decode JWT payload (base64url) — không verify signature ở đây
    // Supabase dùng HS256, verify bằng secret
    bool        verifySignature(const std::string& token) const;
    json        decodePayload(const std::string& token) const;
    std::string base64UrlDecode(const std::string& in) const;
};
