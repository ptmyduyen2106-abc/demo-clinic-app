#pragma once
#include <string>
#include <vector>
#include <functional>
#include "../libs/json.hpp"
#include "../libs/httplib.h"

using json = nlohmann::json;

// ─────────────────────────────────────────────
// QueryBuilder — fluent builder cho Supabase REST
// ─────────────────────────────────────────────
struct QueryParams {
    std::string select;       // "*, users(name)"
    std::string filter;       // "status=eq.pending"
    std::string order;        // "created_at.desc"
    int         limit  = -1;
    int         offset = -1;

    QueryParams& withSelect(std::string s)  { select = std::move(s); return *this; }
    QueryParams& withFilter(std::string f)  { filter = std::move(f); return *this; }
    QueryParams& withOrder(std::string o)   { order  = std::move(o); return *this; }
    QueryParams& withLimit(int l)           { limit  = l;            return *this; }
    QueryParams& withOffset(int o)          { offset = o;            return *this; }

    std::string toQueryString() const {
        std::string q;
        auto add = [&](const std::string& k, const std::string& v) {
            if (!v.empty()) q += (q.empty() ? "?" : "&") + k + "=" + v;
        };
        add("select", select);
        if (!filter.empty()) {
            // filter: "column=op.value" → split on first '='
            auto eq = filter.find('=');
            if (eq != std::string::npos)
                add(filter.substr(0, eq), filter.substr(eq + 1));
        }
        if (!order.empty()) add("order", order);
        if (limit  > 0) add("limit",  std::to_string(limit));
        if (offset > 0) add("offset", std::to_string(offset));
        return q;
    }
};

// ─────────────────────────────────────────────
// DbResult<T> — kết quả trả về từ Supabase
// ─────────────────────────────────────────────
template <typename T>
struct DbResult {
    bool        success = false;
    T           data;
    std::string error;
    int         httpStatus = 200;

    static DbResult<T> ok(T d)                       { return {true, std::move(d), "", 200}; }
    static DbResult<T> err(std::string e, int code=500) { return {false, T{}, std::move(e), code}; }
};

// ─────────────────────────────────────────────
// SupabaseClient — HTTP client gọi Supabase REST API
// Sử dụng cpp-httplib + nlohmann/json
// ─────────────────────────────────────────────
class SupabaseClient {
public:
    // Khởi tạo với URL và service_role_key (backend dùng service key, bỏ RLS)
    SupabaseClient(const std::string& projectUrl,
                   const std::string& serviceRoleKey);

    // ── patient_records ───────────────────────
    DbResult<json>              getPendingQueue();
    DbResult<json>              getAllRecords(int limit = 50);
    DbResult<json>              getRecordById(const std::string& id);
    DbResult<json>              getRecordsByDoctor(const std::string& doctorId);
    DbResult<json>              getRecordsByPatient(const std::string& patientName,
                                                     const std::string& phone = "");
    DbResult<json>              insertRecord(const json& body);
    DbResult<json>              updateRecord(const std::string& id, const json& body);
    DbResult<void>              completeRecord(const std::string& id,
                                               const std::string& pharmacyNote = "");
    DbResult<void>              cancelRecord(const std::string& id);

    // ── users ─────────────────────────────────
    DbResult<json>              getUserById(const std::string& id);
    DbResult<json>              getUsersByRole(const std::string& role);
    DbResult<json>              upsertUser(const json& body);

    // ── expenses ──────────────────────────────
    DbResult<json>              getExpensesByMonth(const std::string& month); // "2024-06"
    DbResult<json>              insertExpense(const json& body);

    // ── inventory ─────────────────────────────
    DbResult<json>              getInventoryByMonth(const std::string& month);
    DbResult<json>              insertInventory(const json& body);

    // ── Generic REST ──────────────────────────
    DbResult<json>              query(const std::string& table,
                                     const QueryParams& params = {});
    DbResult<json>              insert(const std::string& table, const json& body);
    DbResult<json>              update(const std::string& table,
                                      const std::string& filter,
                                      const json& body);
    DbResult<void>              remove(const std::string& table,
                                      const std::string& filter);

private:
    std::string m_host;           // "pvmylmgywyyzuptnbwup.supabase.co"
    std::string m_serviceKey;
    std::string m_basePath;       // "/rest/v1"

    // Build common headers
    httplib::Headers buildHeaders(bool preferCount = false) const;

    // Execute HTTP calls
    DbResult<json> doGet(const std::string& path) const;
    DbResult<json> doPost(const std::string& path, const json& body) const;
    DbResult<json> doPatch(const std::string& path, const json& body) const;
    DbResult<void> doDelete(const std::string& path) const;

    // Date helpers for month filtering
    static std::pair<std::string, std::string> monthRange(const std::string& month);
};
