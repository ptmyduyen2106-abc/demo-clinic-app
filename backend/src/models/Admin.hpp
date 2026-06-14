#pragma once
#include "User.hpp"
#include "Doctor.hpp"
#include "Pharmacist.hpp"

// ─────────────────────────────────────────────
// Admin — quản lý, kế thừa full quyền
// Admin = Doctor quyền + Pharmacist quyền + Finance
// ─────────────────────────────────────────────
class Admin : public User {
public:
    bool canManageStaff       = true;
    bool canViewAllFinance    = true;
    bool canConfigSystem      = true;
    bool canSendBroadcast     = true;
    bool canManageSchedules   = true;
    bool canDeleteRecords     = false;  // Không ai được xóa hồ sơ bệnh nhân

    Admin() { role = UserRole::Admin; }

    explicit Admin(const User& base) : User(base) {
        role = UserRole::Admin;
    }

    // Admin có quyền của Doctor
    bool hasDocumentPermissions() const {
        return true;  // Xem và khám bệnh
    }

    // Admin có quyền của Pharmacist
    bool hasPharmacyPermissions() const {
        return true;  // Phát thuốc, quản lý kho
    }

    json toJson() const override {
        json j = User::toJson();
        j["canManageStaff"]     = canManageStaff;
        j["canViewAllFinance"]  = canViewAllFinance;
        j["canConfigSystem"]    = canConfigSystem;
        j["canSendBroadcast"]   = canSendBroadcast;
        j["canManageSchedules"] = canManageSchedules;
        j["canDeleteRecords"]   = canDeleteRecords;
        return j;
    }

    static Admin fromJson(const json& j) {
        Admin a;
        a.id        = j.value("id", "");
        a.name      = j.value("name", "");
        a.role      = UserRole::Admin;
        a.createdAt = j.value("created_at", "");
        return a;
    }
};
