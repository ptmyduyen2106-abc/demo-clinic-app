#pragma once
#include <string>
#include <vector>
#include "PrescriptionItem.hpp"
#include "Service.hpp"
#include "../libs/json.hpp"

using json = nlohmann::json;

// ─────────────────────────────────────────────
// PatientStatus — trạng thái hồ sơ bệnh nhân
// ─────────────────────────────────────────────
enum class PatientStatus {
    Pending,    // "pending" — chờ nhà thuốc
    Done,       // "done"    — đã phát thuốc & thu tiền
    Cancelled   // "cancelled"
};

// ─────────────────────────────────────────────
// PatientRecord — hồ sơ một lần khám
// Maps to patient_records table
// ─────────────────────────────────────────────
class PatientRecord {
public:
    std::string  id;
    std::string  patientName;
    int          yearOfBirth  = 0;
    double       weight       = 0.0;   // kg
    std::string  phone;
    std::string  diagnosis;

    std::vector<PrescriptionItem> prescription;
    std::vector<Service>          services;
    std::string                   serviceNote;

    double       totalPrice   = 0.0;
    std::string  advice;
    std::string  pharmacyNote;
    PatientStatus status      = PatientStatus::Pending;

    std::string  doctorId;
    std::string  doctorName;   // join từ users

    std::string  createdAt;
    std::string  completedAt;

    // ── Computed ──────────────────────────────
    double drugTotal() const {
        double total = 0;
        for (const auto& item : prescription)
            total += item.lineTotal();
        return total;
    }

    double serviceTotal() const {
        double total = 0;
        for (const auto& svc : services)
            total += svc.price;
        return total;
    }

    void recalcTotal() {
        totalPrice = drugTotal() + serviceTotal();
    }

    int age() const {
        if (yearOfBirth <= 0) return 0;
        // Đơn giản — không cần chrono cho tuổi xấp xỉ
        return 2024 - yearOfBirth;
    }

    // ── Status helpers ────────────────────────
    static PatientStatus statusFromString(const std::string& s) {
        if (s == "done")      return PatientStatus::Done;
        if (s == "cancelled") return PatientStatus::Cancelled;
        return PatientStatus::Pending;
    }

    static std::string statusToString(PatientStatus s) {
        switch (s) {
            case PatientStatus::Done:      return "done";
            case PatientStatus::Cancelled: return "cancelled";
            default:                       return "pending";
        }
    }

    std::string statusStr() const { return statusToString(status); }

    // ── JSON serialization ────────────────────
    json toJson() const {
        json prescJson = json::array();
        for (const auto& p : prescription) prescJson.push_back(p.toJson());

        json svcJson = json::array();
        for (const auto& s : services) svcJson.push_back(s.toJson());

        return {
            {"id",           id},
            {"patientName",  patientName},
            {"yearOfBirth",  yearOfBirth},
            {"weight",       weight},
            {"phone",        phone},
            {"diagnosis",    diagnosis},
            {"prescription", prescJson},
            {"services",     svcJson},
            {"serviceNote",  serviceNote},
            {"totalPrice",   totalPrice},
            {"advice",       advice},
            {"pharmacyNote", pharmacyNote},
            {"status",       statusStr()},
            {"doctorId",     doctorId},
            {"doctorName",   doctorName},
            {"createdAt",    createdAt},
            {"completedAt",  completedAt},
            {"drugTotal",    drugTotal()},
            {"serviceTotal", serviceTotal()}
        };
    }

    // Map snake_case Supabase row → camelCase
    static PatientRecord fromSupabaseRow(const json& row) {
        PatientRecord r;
        r.id          = row.value("id", "");
        r.patientName = row.value("patient_name", "");
        r.yearOfBirth = row.value("year_of_birth", 0);
        r.weight      = row.value("weight", 0.0);
        r.phone       = row.value("phone", "");
        r.diagnosis   = row.value("diagnosis", "");
        r.serviceNote = row.value("service_note", "");
        r.totalPrice  = row.value("total_price", 0.0);
        r.advice      = row.value("advice", "");
        r.pharmacyNote= row.value("pharmacy_note", "");
        r.status      = statusFromString(row.value("status", "pending"));
        r.doctorId    = row.value("doctor_id", "");
        r.createdAt   = row.value("created_at", "");
        r.completedAt = row.value("completed_at", "");

        // Parse prescription JSONB array
        if (row.contains("prescription") && row["prescription"].is_array()) {
            for (const auto& p : row["prescription"])
                r.prescription.push_back(PrescriptionItem::fromJson(p));
        }

        // Parse services JSONB array
        if (row.contains("services") && row["services"].is_array()) {
            for (const auto& s : row["services"])
                r.services.push_back(Service::fromJson(s));
        }

        // Join từ users nếu có
        if (row.contains("users") && row["users"].is_object())
            r.doctorName = row["users"].value("name", "");

        return r;
    }

    // Tạo Supabase insert body (snake_case)
    json toSupabaseInsert() const {
        json prescJson = json::array();
        for (const auto& p : prescription) prescJson.push_back(p.toJson());
        json svcJson = json::array();
        for (const auto& s : services) svcJson.push_back(s.toJson());

        return {
            {"patient_name",  patientName},
            {"year_of_birth", yearOfBirth},
            {"weight",        weight},
            {"phone",         phone},
            {"diagnosis",     diagnosis},
            {"prescription",  prescJson},
            {"services",      svcJson},
            {"service_note",  serviceNote},
            {"total_price",   totalPrice},
            {"advice",        advice},
            {"status",        "pending"},
            {"doctor_id",     doctorId}
        };
    }

    // ── Validation ────────────────────────────
    bool isValid() const {
        return !patientName.empty() && !diagnosis.empty() && !doctorId.empty();
    }

    std::string validationError() const {
        if (patientName.empty()) return "Tên bệnh nhân không được để trống";
        if (diagnosis.empty())   return "Chẩn đoán không được để trống";
        if (doctorId.empty())    return "Thiếu doctor_id";
        for (const auto& item : prescription) {
            auto err = item.validationError();
            if (!err.empty()) return "Thuốc '" + item.drugName + "': " + err;
        }
        return "";
    }
};
