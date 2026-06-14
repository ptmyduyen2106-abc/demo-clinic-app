#pragma once
#include "User.hpp"
#include <vector>

// ─────────────────────────────────────────────
// Doctor — bác sĩ kế thừa từ User
// Có thêm: lịch làm việc, specialization
// ─────────────────────────────────────────────
class Doctor : public User {
public:
    std::string specialization;     // "Nội khoa", "Nhi khoa"…
    std::string licenseNumber;      // Số chứng chỉ hành nghề
    std::vector<std::string> workDays;  // ["Mon","Tue","Wed"]
    std::string workHoursStart;    // "08:00"
    std::string workHoursEnd;      // "17:00"
    int         maxPatientsPerDay = 30;
    bool        isAvailable       = true;

    Doctor() { role = UserRole::Doctor; }

    explicit Doctor(const User& base) : User(base) {
        role = UserRole::Doctor;
    }

    // ── JSON ──────────────────────────────────
    json toJson() const override {
        json j = User::toJson();
        j["specialization"]   = specialization;
        j["licenseNumber"]    = licenseNumber;
        j["workDays"]         = workDays;
        j["workHoursStart"]   = workHoursStart;
        j["workHoursEnd"]     = workHoursEnd;
        j["maxPatientsPerDay"]= maxPatientsPerDay;
        j["isAvailable"]      = isAvailable;
        return j;
    }

    static Doctor fromJson(const json& j) {
        Doctor d;
        d.id        = j.value("id", "");
        d.name      = j.value("name", "");
        d.role      = UserRole::Doctor;
        d.createdAt = j.value("created_at", "");

        if (j.contains("specialization"))
            d.specialization = j["specialization"].get<std::string>();
        if (j.contains("license_number"))
            d.licenseNumber = j["license_number"].get<std::string>();
        if (j.contains("work_hours_start"))
            d.workHoursStart = j["work_hours_start"].get<std::string>();
        if (j.contains("work_hours_end"))
            d.workHoursEnd = j["work_hours_end"].get<std::string>();
        if (j.contains("max_patients_per_day"))
            d.maxPatientsPerDay = j["max_patients_per_day"].get<int>();
        if (j.contains("is_available"))
            d.isAvailable = j["is_available"].get<bool>();
        return d;
    }
};
