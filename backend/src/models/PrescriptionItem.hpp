#pragma once
#include <string>
#include "../libs/json.hpp"

using json = nlohmann::json;

// ─────────────────────────────────────────────
// PrescriptionItem — một dòng thuốc trong toa
// Maps to JSONB array trong patient_records
// ─────────────────────────────────────────────
struct PrescriptionItem {
    std::string drugName;
    std::string dosage;       // "1v x 2/ngày"
    int         quantity   = 1;
    std::string instruction;  // "Uống sau ăn"
    double      unitPrice  = 0.0;

    // ── Computed ──────────────────────────────
    double lineTotal() const { return quantity * unitPrice; }

    // ── JSON serialization ────────────────────
    json toJson() const {
        return {
            {"drugName",    drugName},
            {"dosage",      dosage},
            {"quantity",    quantity},
            {"instruction", instruction},
            {"unitPrice",   unitPrice},
            {"lineTotal",   lineTotal()}
        };
    }

    static PrescriptionItem fromJson(const json& j) {
        PrescriptionItem item;
        if (j.contains("drugName"))    item.drugName    = j["drugName"].get<std::string>();
        if (j.contains("dosage"))      item.dosage      = j["dosage"].get<std::string>();
        if (j.contains("quantity"))    item.quantity    = j["quantity"].get<int>();
        if (j.contains("instruction")) item.instruction = j["instruction"].get<std::string>();
        if (j.contains("unitPrice"))   item.unitPrice   = j["unitPrice"].get<double>();
        return item;
    }

    // ── Validation ────────────────────────────
    bool isValid() const {
        return !drugName.empty() && quantity > 0 && unitPrice >= 0;
    }

    std::string validationError() const {
        if (drugName.empty())  return "Tên thuốc không được để trống";
        if (quantity <= 0)     return "Số lượng phải lớn hơn 0";
        if (unitPrice < 0)     return "Đơn giá không được âm";
        return "";
    }
};
