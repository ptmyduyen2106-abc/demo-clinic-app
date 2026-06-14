#pragma once
#include "User.hpp"

// ─────────────────────────────────────────────
// Pharmacist — dược sĩ kế thừa từ User
// Có thêm: quyền cập nhật toa, nhập kho
// ─────────────────────────────────────────────
class Pharmacist : public User {
public:
    std::string licenseNumber;
    bool canEditPrescription  = true;   // Quyền chỉnh sửa toa thuốc
    bool canManageInventory   = true;   // Quyền nhập xuất kho
    bool canViewFinancials    = false;  // Không được xem tài chính tổng

    Pharmacist() { role = UserRole::Pharmacist; }

    explicit Pharmacist(const User& base) : User(base) {
        role = UserRole::Pharmacist;
    }

    json toJson() const override {
        json j = User::toJson();
        j["licenseNumber"]       = licenseNumber;
        j["canEditPrescription"] = canEditPrescription;
        j["canManageInventory"]  = canManageInventory;
        j["canViewFinancials"]   = canViewFinancials;
        return j;
    }

    static Pharmacist fromJson(const json& j) {
        Pharmacist p;
        p.id        = j.value("id", "");
        p.name      = j.value("name", "");
        p.role      = UserRole::Pharmacist;
        p.createdAt = j.value("created_at", "");
        if (j.contains("license_number"))
            p.licenseNumber = j["license_number"].get<std::string>();
        return p;
    }
};
