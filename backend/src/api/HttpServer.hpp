#pragma once
#include "../libs/httplib.h"
#include "../libs/json.hpp"
#include "../utils/AuthManager.hpp"
#include "../utils/ApiResponse.hpp"
#include "SupabaseClient.hpp"
#include <string>
#include <memory>

using json = nlohmann::json;

// ─────────────────────────────────────────────
// HttpServer — định nghĩa toàn bộ routes
//
// Routes layout:
//   /health                     GET  — public
//
//   /api/auth/me                GET  — any auth
//
//   /api/records                GET  — doctor | admin
//   /api/records                POST — doctor | admin
//   /api/records/:id            GET  — doctor | pharma | admin
//   /api/records/:id/complete   PATCH — pharma | admin
//   /api/records/:id/cancel     PATCH — doctor | admin
//   /api/records/queue          GET  — pharma | admin
//   /api/records/search         GET  — doctor | admin (by patient name/phone)
//
//   /api/users                  GET  — admin
//   /api/users/:id              GET  — admin
//   /api/users                  POST — admin
//
//   /api/finance/expenses       GET  — admin
//   /api/finance/expenses       POST — admin
//   /api/finance/inventory      GET  — admin | pharma
//   /api/finance/inventory      POST — admin | pharma
//   /api/finance/summary        GET  — admin
//
// ─────────────────────────────────────────────
class HttpServer {
public:
    HttpServer(int port,
               std::shared_ptr<SupabaseClient> db,
               std::shared_ptr<AuthManager>    auth);

    void start();   // blocking

private:
    int                             m_port;
    std::shared_ptr<SupabaseClient> m_db;
    std::shared_ptr<AuthManager>    m_auth;
    httplib::Server                 m_srv;

    void setupCors();
    void setupRoutes();

    // ── Helpers ───────────────────────────────
    void sendJson(httplib::Response& res, const std::string& body, int status = 200);
    void sendError(httplib::Response& res, const std::string& msg, int status);

    // ── Route handlers ────────────────────────
    // Health
    void handleHealth(const httplib::Request&, httplib::Response&);

    // Auth
    void handleGetMe(const httplib::Request&, httplib::Response&);

    // Records
    void handleGetQueue(const httplib::Request&, httplib::Response&);
    void handleGetRecords(const httplib::Request&, httplib::Response&);
    void handleGetRecordById(const httplib::Request&, httplib::Response&);
    void handleCreateRecord(const httplib::Request&, httplib::Response&);
    void handleCompleteRecord(const httplib::Request&, httplib::Response&);
    void handleCancelRecord(const httplib::Request&, httplib::Response&);
    void handleSearchRecords(const httplib::Request&, httplib::Response&);

    // Users
    void handleGetUsers(const httplib::Request&, httplib::Response&);
    void handleGetUserById(const httplib::Request&, httplib::Response&);
    void handleCreateUser(const httplib::Request&, httplib::Response&);

    // Finance
    void handleGetExpenses(const httplib::Request&, httplib::Response&);
    void handleCreateExpense(const httplib::Request&, httplib::Response&);
    void handleGetInventory(const httplib::Request&, httplib::Response&);
    void handleCreateInventory(const httplib::Request&, httplib::Response&);
    void handleGetFinanceSummary(const httplib::Request&, httplib::Response&);
};
