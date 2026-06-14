#include "SupabaseClient.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

// ─────────────────────────────────────────────
// SupabaseClient — implementation
// ─────────────────────────────────────────────

SupabaseClient::SupabaseClient(const std::string& projectUrl,
                               const std::string& serviceRoleKey)
    : m_serviceKey(serviceRoleKey)
    , m_basePath("/rest/v1")
{
    // Extract host từ URL: "https://xxxx.supabase.co" → "xxxx.supabase.co"
    m_host = projectUrl;
    if (m_host.substr(0, 8) == "https://") m_host = m_host.substr(8);
    if (m_host.back() == '/')              m_host.pop_back();
}

// ── Headers ───────────────────────────────────
httplib::Headers SupabaseClient::buildHeaders(bool preferCount) const {
    httplib::Headers h;
    h.insert({"apikey",        m_serviceKey});
    h.insert({"Authorization", "Bearer " + m_serviceKey});
    h.insert({"Content-Type",  "application/json"});
    h.insert({"Accept",        "application/json"});
    if (preferCount)
        h.insert({"Prefer", "count=exact"});
    return h;
}

// ── Generic REST operations ───────────────────
DbResult<json> SupabaseClient::doGet(const std::string& path) const {
    httplib::SSLClient cli(m_host);
    cli.set_connection_timeout(10);
    cli.set_read_timeout(15);

    auto res = cli.Get(path.c_str(), buildHeaders());
    if (!res) return DbResult<json>::err("Network error: no response", 503);

    if (res->status >= 400) {
        std::string msg = "HTTP " + std::to_string(res->status);
        try {
            auto errJson = json::parse(res->body);
            if (errJson.contains("message"))
                msg = errJson["message"].get<std::string>();
        } catch (...) {}
        return DbResult<json>::err(msg, res->status);
    }

    try {
        return DbResult<json>::ok(json::parse(res->body));
    } catch (const std::exception& e) {
        return DbResult<json>::err("JSON parse error: " + std::string(e.what()));
    }
}

DbResult<json> SupabaseClient::doPost(const std::string& path, const json& body) const {
    httplib::SSLClient cli(m_host);
    cli.set_connection_timeout(10);

    auto headers = buildHeaders();
    headers.insert({"Prefer", "return=representation"});

    auto res = cli.Post(path.c_str(), headers, body.dump(), "application/json");
    if (!res) return DbResult<json>::err("Network error", 503);

    if (res->status >= 400) {
        try {
            auto errJson = json::parse(res->body);
            return DbResult<json>::err(errJson.value("message", "Unknown error"), res->status);
        } catch (...) {}
        return DbResult<json>::err("HTTP " + std::to_string(res->status), res->status);
    }

    try {
        return DbResult<json>::ok(json::parse(res->body));
    } catch (const std::exception& e) {
        return DbResult<json>::err(e.what());
    }
}

DbResult<json> SupabaseClient::doPatch(const std::string& path, const json& body) const {
    httplib::SSLClient cli(m_host);
    cli.set_connection_timeout(10);

    auto headers = buildHeaders();
    headers.insert({"Prefer", "return=representation"});

    auto res = cli.Patch(path.c_str(), headers, body.dump(), "application/json");
    if (!res) return DbResult<json>::err("Network error", 503);

    if (res->status >= 400)
        return DbResult<json>::err("HTTP " + std::to_string(res->status), res->status);

    try {
        return DbResult<json>::ok(json::parse(res->body));
    } catch (...) {
        return DbResult<json>::ok(json::object());
    }
}

DbResult<void> SupabaseClient::doDelete(const std::string& path) const {
    httplib::SSLClient cli(m_host);
    auto res = cli.Delete(path.c_str(), buildHeaders());
    if (!res) return DbResult<void>::err("Network error", 503);
    if (res->status >= 400)
        return DbResult<void>::err("HTTP " + std::to_string(res->status), res->status);
    return DbResult<void>::ok({});
}

// ── Generic query builder ─────────────────────
DbResult<json> SupabaseClient::query(const std::string& table,
                                     const QueryParams& params) {
    std::string path = m_basePath + "/" + table + params.toQueryString();
    return doGet(path);
}

DbResult<json> SupabaseClient::insert(const std::string& table, const json& body) {
    return doPost(m_basePath + "/" + table, body);
}

DbResult<json> SupabaseClient::update(const std::string& table,
                                      const std::string& filter,
                                      const json& body) {
    return doPatch(m_basePath + "/" + table + "?" + filter, body);
}

DbResult<void> SupabaseClient::remove(const std::string& table,
                                      const std::string& filter) {
    return doDelete(m_basePath + "/" + table + "?" + filter);
}

// ── patient_records ───────────────────────────
DbResult<json> SupabaseClient::getPendingQueue() {
    QueryParams p;
    p.withSelect("*, users(name)")
     .withFilter("status=eq.pending")
     .withOrder("created_at.desc");
    return query("patient_records", p);
}

DbResult<json> SupabaseClient::getAllRecords(int limit) {
    QueryParams p;
    p.withSelect("*, users(name)")
     .withOrder("created_at.desc")
     .withLimit(limit);
    return query("patient_records", p);
}

DbResult<json> SupabaseClient::getRecordById(const std::string& id) {
    std::string path = m_basePath + "/patient_records?select=*,users(name)&id=eq." + id;
    return doGet(path);
}

DbResult<json> SupabaseClient::getRecordsByDoctor(const std::string& doctorId) {
    QueryParams p;
    p.withSelect("*")
     .withFilter("doctor_id=eq." + doctorId)
     .withOrder("created_at.desc")
     .withLimit(100);
    return query("patient_records", p);
}

DbResult<json> SupabaseClient::getRecordsByPatient(const std::string& patientName,
                                                    const std::string& phone) {
    std::string path = m_basePath + "/patient_records?select=*,users(name)";
    // Supabase REST: ilike for case-insensitive search
    path += "&patient_name=ilike.*" + patientName + "*";
    if (!phone.empty()) path += "&phone=eq." + phone;
    path += "&order=created_at.desc";
    return doGet(path);
}

DbResult<json> SupabaseClient::insertRecord(const json& body) {
    return insert("patient_records", body);
}

DbResult<json> SupabaseClient::updateRecord(const std::string& id, const json& body) {
    return update("patient_records", "id=eq." + id, body);
}

DbResult<void> SupabaseClient::completeRecord(const std::string& id,
                                               const std::string& pharmacyNote) {
    json body = {
        {"status",         "done"},
        {"pharmacy_note",  pharmacyNote},
        {"completed_at",   "now()"}
    };
    auto res = update("patient_records", "id=eq." + id, body);
    if (!res.success) return DbResult<void>::err(res.error, res.httpStatus);
    return DbResult<void>::ok({});
}

DbResult<void> SupabaseClient::cancelRecord(const std::string& id) {
    json body = {{"status", "cancelled"}};
    auto res = update("patient_records", "id=eq." + id, body);
    if (!res.success) return DbResult<void>::err(res.error, res.httpStatus);
    return DbResult<void>::ok({});
}

// ── users ─────────────────────────────────────
DbResult<json> SupabaseClient::getUserById(const std::string& id) {
    return doGet(m_basePath + "/users?id=eq." + id);
}

DbResult<json> SupabaseClient::getUsersByRole(const std::string& role) {
    return doGet(m_basePath + "/users?role=eq." + role);
}

DbResult<json> SupabaseClient::upsertUser(const json& body) {
    httplib::SSLClient cli(m_host);
    auto headers = buildHeaders();
    headers.insert({"Prefer", "resolution=merge-duplicates,return=representation"});
    auto res = cli.Post((m_basePath + "/users").c_str(), headers, body.dump(), "application/json");
    if (!res) return DbResult<json>::err("Network error", 503);
    try { return DbResult<json>::ok(json::parse(res->body)); }
    catch (...) { return DbResult<json>::err("Parse error"); }
}

// ── expenses ──────────────────────────────────
DbResult<json> SupabaseClient::getExpensesByMonth(const std::string& month) {
    auto [start, end] = monthRange(month);
    std::string path = m_basePath + "/expenses?date=gte." + start + "&date=lte." + end
                     + "&order=date.desc";
    return doGet(path);
}

DbResult<json> SupabaseClient::insertExpense(const json& body) {
    return insert("expenses", body);
}

// ── inventory ─────────────────────────────────
DbResult<json> SupabaseClient::getInventoryByMonth(const std::string& month) {
    auto [start, end] = monthRange(month);
    std::string path = m_basePath + "/inventory?date=gte." + start + "&date=lte." + end
                     + "&order=date.desc";
    return doGet(path);
}

DbResult<json> SupabaseClient::insertInventory(const json& body) {
    return insert("inventory", body);
}

// ── Date helpers ──────────────────────────────
std::pair<std::string, std::string>
SupabaseClient::monthRange(const std::string& month) {
    // month = "2024-06"
    std::string start = month + "-01";
    // Calculate last day
    int year = std::stoi(month.substr(0, 4));
    int mon  = std::stoi(month.substr(5, 2));
    int lastDay[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (mon == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0))
        lastDay[2] = 29;
    char end[12];
    snprintf(end, sizeof(end), "%04d-%02d-%02d", year, mon, lastDay[mon]);
    return {start, std::string(end)};
}
