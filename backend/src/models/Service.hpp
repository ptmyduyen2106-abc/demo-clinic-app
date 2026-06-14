#pragma once
#include <string>
#include "../libs/json.hpp"

using json = nlohmann::json;

// ─────────────────────────────────────────────
// Service — dịch vụ khám (khám bệnh, xét nghiệm…)
// Maps to JSONB array trong patient_records
// ─────────────────────────────────────────────
struct Service {
    std::string name;
    double      price = 0.0;

    // ── JSON serialization ────────────────────
    json toJson() const {
        return {{"name", name}, {"price", price}};
    }

    static Service fromJson(const json& j) {
        Service s;
        if (j.contains("name"))  s.name  = j["name"].get<std::string>();
        if (j.contains("price")) s.price = j["price"].get<double>();
        return s;
    }

    bool isValid() const { return !name.empty() && price >= 0; }
};

// ─────────────────────────────────────────────
// ServicePreset — danh sách dịch vụ mặc định
// Tham chiếu từ DoctorForm.tsx
// ─────────────────────────────────────────────
namespace ServicePresets {
    inline std::vector<Service> defaults() {
        return {
            {"Khám bệnh",       100000},
            {"Tái khám",         50000},
            {"Khám chuyên khoa",150000},
            {"Siêu âm",         120000},
            {"Xét nghiệm máu",  200000},
            {"Điện tâm đồ",      80000},
        };
    }
}
