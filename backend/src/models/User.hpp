#pragma once
#include <string>
#include "../libs/json.hpp"
#include "../utils/AuthManager.hpp"

using json = nlohmann::json;

// ─────────────────────────────────────────────
// User — base class cho mọi tài khoản hệ thống
// Maps to public.users table in Supabase
// ─────────────────────────────────────────────
class User {
public:
    std::string id;          // UUID từ auth.users
    std::string name;
    UserRole    role;
    std::string createdAt;

    User() : role(UserRole::Unknown) {}

    User(std::string id, std::string name, UserRole role, std::string createdAt = "")
        : id(std::move(id))
        , name(std::move(name))
        , role(role)
        , createdAt(std::move(createdAt)) {}

    virtual ~User() = default;

    // ── Getters ───────────────────────────────
    std::string roleString() const { return AuthManager::roleToString(role); }

    // ── JSON ──────────────────────────────────
    virtual json toJson() const {
        return {
            {"id",        id},
            {"name",      name},
            {"role",      roleString()},
            {"createdAt", createdAt}
        };
    }

    static User fromJson(const json& j) {
        User u;
        if (j.contains("id"))         u.id        = j["id"].get<std::string>();
        if (j.contains("name"))       u.name      = j["name"].get<std::string>();
        if (j.contains("role"))       u.role      = AuthManager::roleFromString(
                                                       j["role"].get<std::string>());
        if (j.contains("created_at")) u.createdAt = j["created_at"].get<std::string>();
        return u;
    }

    // ── Permission helpers ────────────────────
    bool isAdmin()      const { return role == UserRole::Admin; }
    bool isDoctor()     const { return role == UserRole::Doctor; }
    bool isPharmacist() const { return role == UserRole::Pharmacist; }
    bool isPatient()    const { return role == UserRole::Patient; }

    bool isValid() const { return !id.empty() && role != UserRole::Unknown; }
};
