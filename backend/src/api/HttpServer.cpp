#include "HttpServer.hpp"
#include "../models/PatientRecord.hpp"
#include "../models/User.hpp"
#include <iostream>
#include <sstream>

// ─────────────────────────────────────────────
// HttpServer — implementation
// ─────────────────────────────────────────────

HttpServer::HttpServer(int port,
                       std::shared_ptr<SupabaseClient> db,
                       std::shared_ptr<AuthManager>    auth)
    : m_port(port), m_db(std::move(db)), m_auth(std::move(auth))
{}

void HttpServer::start() {
    setupCors();
    setupRoutes();

    std::cout << "[HttpServer] Listening on port " << m_port << "\n";
    m_srv.listen("0.0.0.0", m_port);
}

// ── CORS ──────────────────────────────────────
void HttpServer::setupCors() {
    m_srv.set_pre_routing_handler(
        [](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin",  "*");
            res.set_header("Access-Control-Allow-Methods", "GET,POST,PATCH,DELETE,OPTIONS");
            res.set_header("Access-Control-Allow-Headers",
                           "Content-Type, Authorization");
            if (req.method == "OPTIONS") {
                res.status = 204;
                return httplib::Server::HandlerResponse::Handled;
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });
}

// ── Route registration ────────────────────────
void HttpServer::setupRoutes() {
    using Req = const httplib::Request&;
    using Res = httplib::Response&;

    // Health check — public
    m_srv.Get("/health", [this](Req q, Res r){ handleHealth(q, r); });

    // Auth
    m_srv.Get("/api/auth/me", [this](Req q, Res r){ handleGetMe(q, r); });

    // Patient records
    m_srv.Get ("/api/records/queue",       [this](Req q, Res r){ handleGetQueue(q, r); });
    m_srv.Get ("/api/records/search",      [this](Req q, Res r){ handleSearchRecords(q, r); });
    m_srv.Get ("/api/records",             [this](Req q, Res r){ handleGetRecords(q, r); });
    m_srv.Get ("/api/records/:id",         [this](Req q, Res r){ handleGetRecordById(q, r); });
    m_srv.Post("/api/records",             [this](Req q, Res r){ handleCreateRecord(q, r); });
    m_srv.Patch("/api/records/:id/complete",[this](Req q, Res r){ handleCompleteRecord(q, r); });
    m_srv.Patch("/api/records/:id/cancel",  [this](Req q, Res r){ handleCancelRecord(q, r); });

    // Users
    m_srv.Get ("/api/users",     [this](Req q, Res r){ handleGetUsers(q, r); });
    m_srv.Get ("/api/users/:id", [this](Req q, Res r){ handleGetUserById(q, r); });
    m_srv.Post("/api/users",     [this](Req q, Res r){ handleCreateUser(q, r); });

    // Finance
    m_srv.Get ("/api/finance/expenses",   [this](Req q, Res r){ handleGetExpenses(q, r); });
    m_srv.Post("/api/finance/expenses",   [this](Req q, Res r){ handleCreateExpense(q, r); });
    m_srv.Get ("/api/finance/inventory",  [this](Req q, Res r){ handleGetInventory(q, r); });
    m_srv.Post("/api/finance/inventory",  [this](Req q, Res r){ handleCreateInventory(q, r); });
    m_srv.Get ("/api/finance/summary",    [this](Req q, Res r){ handleGetFinanceSummary(q, r); });
}

// ── Helpers ───────────────────────────────────
void HttpServer::sendJson(httplib::Response& res,
                          const std::string& body, int status) {
    res.status = status;
    res.set_content(body, "application/json");
}

void HttpServer::sendError(httplib::Response& res,
                           const std::string& msg, int status) {
    auto r = ApiResponse<void>::error(msg, status);
    sendJson(res, r.toJson(), status);
}

// ─────────────────────────────────────────────
// GET /health
// ─────────────────────────────────────────────
void HttpServer::handleHealth(const httplib::Request&,
                               httplib::Response& res) {
    json body = {
        {"status",  "ok"},
        {"service", "clinic-backend"},
        {"version", "1.0.0"}
    };
    sendJson(res, ApiResponse<json>::ok(body, "healthy").toJson());
}

// ─────────────────────────────────────────────
// GET /api/auth/me
// ─────────────────────────────────────────────
void HttpServer::handleGetMe(const httplib::Request& req,
                              httplib::Response& res) {
    auto claims = m_auth->requireAuth(req, res);
    if (!claims.valid) return;

    auto result = m_db->getUserById(claims.userId);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    json data = result.data;
    if (data.is_array() && !data.empty()) data = data[0];

    sendJson(res, ApiResponse<json>::ok(data).toJson());
}

// ─────────────────────────────────────────────
// GET /api/records/queue   — pharma | admin
// Returns only pending records, ordered newest first
// ─────────────────────────────────────────────
void HttpServer::handleGetQueue(const httplib::Request& req,
                                 httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessPharmacy(); });
    if (!claims.valid) return;

    auto result = m_db->getPendingQueue();
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    // Map rows → PatientRecord objects, then back to JSON array
    json out = json::array();
    for (const auto& row : result.data)
        out.push_back(PatientRecord::fromSupabaseRow(row).toJson());

    sendJson(res, ApiResponse<json>::ok(out).toJson());
}

// ─────────────────────────────────────────────
// GET /api/records?limit=50   — doctor | admin
// ─────────────────────────────────────────────
void HttpServer::handleGetRecords(const httplib::Request& req,
                                   httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessDoctor(); });
    if (!claims.valid) return;

    int limit = 50;
    if (req.has_param("limit")) {
        try { limit = std::stoi(req.get_param_value("limit")); }
        catch (...) {}
    }

    // Doctor chỉ xem record của mình, admin xem tất cả
    DbResult<json> result;
    if (claims.isAdmin()) {
        result = m_db->getAllRecords(limit);
    } else {
        result = m_db->getRecordsByDoctor(claims.userId);
    }

    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    json out = json::array();
    for (const auto& row : result.data)
        out.push_back(PatientRecord::fromSupabaseRow(row).toJson());

    sendJson(res, ApiResponse<json>::ok(out).toJson());
}

// ─────────────────────────────────────────────
// GET /api/records/:id   — doctor | pharma | admin
// ─────────────────────────────────────────────
void HttpServer::handleGetRecordById(const httplib::Request& req,
                                      httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){
            return c.canAccessDoctor() || c.canAccessPharmacy();
        });
    if (!claims.valid) return;

    std::string id = req.path_params.at("id");
    auto result = m_db->getRecordById(id);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    json rows = result.data;
    if (!rows.is_array() || rows.empty()) {
        sendError(res, "Record not found", 404);
        return;
    }

    auto record = PatientRecord::fromSupabaseRow(rows[0]);

    // Doctor chỉ xem record của mình (trừ admin)
    if (claims.isDoctor() && record.doctorId != claims.userId) {
        sendError(res, "Forbidden: not your record", 403);
        return;
    }

    sendJson(res, ApiResponse<json>::ok(record.toJson()).toJson());
}

// ─────────────────────────────────────────────
// POST /api/records   — doctor | admin
// Body: { patientName, yearOfBirth, diagnosis, prescription[], services[], … }
// ─────────────────────────────────────────────
void HttpServer::handleCreateRecord(const httplib::Request& req,
                                     httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessDoctor(); });
    if (!claims.valid) return;

    json body;
    try {
        body = json::parse(req.body);
    } catch (...) {
        sendError(res, "Invalid JSON body", 400);
        return;
    }

    // Build PatientRecord from request
    PatientRecord record;
    record.patientName = body.value("patientName", "");
    record.yearOfBirth = body.value("yearOfBirth", 0);
    record.weight      = body.value("weight", 0.0);
    record.phone       = body.value("phone", "");
    record.diagnosis   = body.value("diagnosis", "");
    record.serviceNote = body.value("serviceNote", "");
    record.advice      = body.value("advice", "");
    record.doctorId    = claims.userId;  // Luôn set từ token, không tin body

    // Parse prescription
    if (body.contains("prescription") && body["prescription"].is_array()) {
        for (const auto& p : body["prescription"])
            record.prescription.push_back(PrescriptionItem::fromJson(p));
    }

    // Parse services
    if (body.contains("services") && body["services"].is_array()) {
        for (const auto& s : body["services"])
            record.services.push_back(Service::fromJson(s));
    }

    // Tính lại totalPrice từ data thực tế (không tin client)
    record.recalcTotal();

    // Validate
    std::string valErr = record.validationError();
    if (!valErr.empty()) {
        sendError(res, valErr, 400);
        return;
    }

    auto result = m_db->insertRecord(record.toSupabaseInsert());
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    json inserted = result.data;
    if (inserted.is_array() && !inserted.empty())
        inserted = inserted[0];

    sendJson(res, ApiResponse<json>::created(
        PatientRecord::fromSupabaseRow(inserted).toJson(),
        "Record created"
    ).toJson(), 201);
}

// ─────────────────────────────────────────────
// PATCH /api/records/:id/complete   — pharma | admin
// Body (optional): { pharmacyNote: "…" }
// ─────────────────────────────────────────────
void HttpServer::handleCompleteRecord(const httplib::Request& req,
                                       httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessPharmacy(); });
    if (!claims.valid) return;

    std::string id = req.path_params.at("id");
    std::string note;

    if (!req.body.empty()) {
        try {
            auto body = json::parse(req.body);
            note = body.value("pharmacyNote", "");
        } catch (...) {}
    }

    auto result = m_db->completeRecord(id, note);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    sendJson(res, ApiResponse<void>::ok("Record completed").toJson());
}

// ─────────────────────────────────────────────
// PATCH /api/records/:id/cancel   — doctor | admin
// ─────────────────────────────────────────────
void HttpServer::handleCancelRecord(const httplib::Request& req,
                                     httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessDoctor(); });
    if (!claims.valid) return;

    std::string id = req.path_params.at("id");

    // Doctor chỉ cancel record của mình
    if (!claims.isAdmin()) {
        auto check = m_db->getRecordById(id);
        if (check.success && check.data.is_array() && !check.data.empty()) {
            std::string doctorId = check.data[0].value("doctor_id", "");
            if (doctorId != claims.userId) {
                sendError(res, "Forbidden: not your record", 403);
                return;
            }
        }
    }

    auto result = m_db->cancelRecord(id);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    sendJson(res, ApiResponse<void>::ok("Record cancelled").toJson());
}

// ─────────────────────────────────────────────
// GET /api/records/search?name=…&phone=…   — doctor | admin
// ─────────────────────────────────────────────
void HttpServer::handleSearchRecords(const httplib::Request& req,
                                      httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessDoctor(); });
    if (!claims.valid) return;

    std::string name  = req.has_param("name")  ? req.get_param_value("name")  : "";
    std::string phone = req.has_param("phone") ? req.get_param_value("phone") : "";

    if (name.empty() && phone.empty()) {
        sendError(res, "Provide 'name' or 'phone' query param", 400);
        return;
    }

    auto result = m_db->getRecordsByPatient(name, phone);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    json out = json::array();
    for (const auto& row : result.data)
        out.push_back(PatientRecord::fromSupabaseRow(row).toJson());

    sendJson(res, ApiResponse<json>::ok(out).toJson());
}

// ─────────────────────────────────────────────
// GET /api/users?role=doctor   — admin only
// ─────────────────────────────────────────────
void HttpServer::handleGetUsers(const httplib::Request& req,
                                 httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.isAdmin(); });
    if (!claims.valid) return;

    DbResult<json> result;
    if (req.has_param("role")) {
        result = m_db->getUsersByRole(req.get_param_value("role"));
    } else {
        result = m_db->query("users", QueryParams{}.withOrder("created_at.desc"));
    }

    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    sendJson(res, ApiResponse<json>::ok(result.data).toJson());
}

// ─────────────────────────────────────────────
// GET /api/users/:id   — admin only
// ─────────────────────────────────────────────
void HttpServer::handleGetUserById(const httplib::Request& req,
                                    httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.isAdmin(); });
    if (!claims.valid) return;

    std::string id = req.path_params.at("id");
    auto result = m_db->getUserById(id);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    json data = result.data;
    if (data.is_array() && data.empty()) {
        sendError(res, "User not found", 404);
        return;
    }
    if (data.is_array()) data = data[0];

    sendJson(res, ApiResponse<json>::ok(data).toJson());
}

// ─────────────────────────────────────────────
// POST /api/users   — admin only
// Body: { id, name, role }
// ─────────────────────────────────────────────
void HttpServer::handleCreateUser(const httplib::Request& req,
                                   httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.isAdmin(); });
    if (!claims.valid) return;

    json body;
    try { body = json::parse(req.body); }
    catch (...) { sendError(res, "Invalid JSON", 400); return; }

    // Validate required fields
    if (!body.contains("id") || !body.contains("name") || !body.contains("role")) {
        sendError(res, "Required: id, name, role", 400);
        return;
    }

    // Validate role value
    std::string role = body["role"].get<std::string>();
    if (role != "doctor" && role != "pharma" && role != "admin" && role != "patient") {
        sendError(res, "Role must be: doctor | pharma | admin | patient", 400);
        return;
    }

    auto result = m_db->upsertUser(body);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    json inserted = result.data;
    if (inserted.is_array() && !inserted.empty()) inserted = inserted[0];

    sendJson(res, ApiResponse<json>::created(inserted, "User created").toJson(), 201);
}

// ─────────────────────────────────────────────
// GET /api/finance/expenses?month=2024-06   — admin
// ─────────────────────────────────────────────
void HttpServer::handleGetExpenses(const httplib::Request& req,
                                    httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessFinance(); });
    if (!claims.valid) return;

    std::string month = req.has_param("month")
        ? req.get_param_value("month")
        : "";   // empty = current month (SupabaseClient handles)

    // Default to current month if not provided
    if (month.empty()) {
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now{};
        localtime_r(&t, &tm_now);
        char buf[8];
        snprintf(buf, sizeof(buf), "%04d-%02d", tm_now.tm_year + 1900, tm_now.tm_mon + 1);
        month = buf;
    }

    auto result = m_db->getExpensesByMonth(month);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    sendJson(res, ApiResponse<json>::ok(result.data).toJson());
}

// ─────────────────────────────────────────────
// POST /api/finance/expenses   — admin
// Body: { date, amount, detail, created_by }
// ─────────────────────────────────────────────
void HttpServer::handleCreateExpense(const httplib::Request& req,
                                      httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessFinance(); });
    if (!claims.valid) return;

    json body;
    try { body = json::parse(req.body); }
    catch (...) { sendError(res, "Invalid JSON", 400); return; }

    if (!body.contains("date") || !body.contains("amount") || !body.contains("detail")) {
        sendError(res, "Required: date, amount, detail", 400);
        return;
    }

    // Force created_by từ token
    body["created_by"] = claims.userId;

    auto result = m_db->insertExpense(body);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    json inserted = result.data;
    if (inserted.is_array() && !inserted.empty()) inserted = inserted[0];
    sendJson(res, ApiResponse<json>::created(inserted, "Expense recorded").toJson(), 201);
}

// ─────────────────────────────────────────────
// GET /api/finance/inventory?month=2024-06   — admin | pharma
// ─────────────────────────────────────────────
void HttpServer::handleGetInventory(const httplib::Request& req,
                                     httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessPharmacy(); });
    if (!claims.valid) return;

    std::string month = req.has_param("month")
        ? req.get_param_value("month") : "";

    if (month.empty()) {
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now{};
        localtime_r(&t, &tm_now);
        char buf[8];
        snprintf(buf, sizeof(buf), "%04d-%02d", tm_now.tm_year + 1900, tm_now.tm_mon + 1);
        month = buf;
    }

    auto result = m_db->getInventoryByMonth(month);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    sendJson(res, ApiResponse<json>::ok(result.data).toJson());
}

// ─────────────────────────────────────────────
// POST /api/finance/inventory   — admin | pharma
// ─────────────────────────────────────────────
void HttpServer::handleCreateInventory(const httplib::Request& req,
                                        httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessPharmacy(); });
    if (!claims.valid) return;

    json body;
    try { body = json::parse(req.body); }
    catch (...) { sendError(res, "Invalid JSON", 400); return; }

    if (!body.contains("date") || !body.contains("item_type") || !body.contains("amount")) {
        sendError(res, "Required: date, item_type, amount", 400);
        return;
    }

    auto result = m_db->insertInventory(body);
    if (!result.success) {
        sendError(res, result.error, result.httpStatus);
        return;
    }

    json inserted = result.data;
    if (inserted.is_array() && !inserted.empty()) inserted = inserted[0];
    sendJson(res, ApiResponse<json>::created(inserted, "Inventory item added").toJson(), 201);
}

// ─────────────────────────────────────────────
// GET /api/finance/summary?month=2024-06   — admin
// Aggregate: revenue, expenses, inventory, net profit
// ─────────────────────────────────────────────
void HttpServer::handleGetFinanceSummary(const httplib::Request& req,
                                          httplib::Response& res) {
    auto claims = m_auth->requireRole(req, res,
        [](const AuthClaims& c){ return c.canAccessFinance(); });
    if (!claims.valid) return;

    std::string month = req.has_param("month")
        ? req.get_param_value("month") : "";

    if (month.empty()) {
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now{};
        localtime_r(&t, &tm_now);
        char buf[8];
        snprintf(buf, sizeof(buf), "%04d-%02d", tm_now.tm_year + 1900, tm_now.tm_mon + 1);
        month = buf;
    }

    // Fetch tất cả 3 nguồn song song (sequential cho đơn giản)
    auto recResult  = m_db->getAllRecords(500);   // tháng hiện tại đủ
    auto expResult  = m_db->getExpensesByMonth(month);
    auto invResult  = m_db->getInventoryByMonth(month);

    double totalRevenue   = 0;
    int    patientCount   = 0;
    double totalExpenses  = 0;
    double totalInventory = 0;

    if (recResult.success) {
        for (const auto& row : recResult.data) {
            if (row.value("status", "") == "done") {
                totalRevenue += row.value("total_price", 0.0);
                patientCount++;
            }
        }
    }
    if (expResult.success) {
        for (const auto& row : expResult.data)
            totalExpenses += row.value("amount", 0.0);
    }
    if (invResult.success) {
        for (const auto& row : invResult.data)
            totalInventory += row.value("amount", 0.0);
    }

    json summary = {
        {"month",          month},
        {"totalRevenue",   totalRevenue},
        {"totalExpenses",  totalExpenses},
        {"totalInventory", totalInventory},
        {"netProfit",      totalRevenue - totalExpenses - totalInventory},
        {"patientCount",   patientCount}
    };

    sendJson(res, ApiResponse<json>::ok(summary).toJson());
}
