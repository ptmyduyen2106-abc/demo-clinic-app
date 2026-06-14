#pragma once
#include <string>
#include "../libs/json.hpp"

using json = nlohmann::json;

// ─────────────────────────────────────────────
// ApiResponse<T> — Template wrapper cho mọi response
// Usage:
//   ApiResponse<json>::success(data)
//   ApiResponse<json>::error("Not found", 404)
// ─────────────────────────────────────────────

template <typename T>
class ApiResponse {
public:
    bool        success;
    T           data;
    std::string message;
    int         statusCode;

    // ── Constructors ──────────────────────────
    ApiResponse(bool ok, T d, std::string msg, int code)
        : success(ok), data(d), message(std::move(msg)), statusCode(code) {}

    // ── Static factories ──────────────────────
    static ApiResponse<T> ok(T d, std::string msg = "OK") {
        return ApiResponse<T>(true, d, std::move(msg), 200);
    }

    static ApiResponse<T> created(T d, std::string msg = "Created") {
        return ApiResponse<T>(true, d, std::move(msg), 201);
    }

    static ApiResponse<T> error(std::string msg, int code = 400) {
        return ApiResponse<T>(false, T{}, std::move(msg), code);
    }

    static ApiResponse<T> notFound(std::string msg = "Not found") {
        return ApiResponse<T>(false, T{}, std::move(msg), 404);
    }

    static ApiResponse<T> unauthorized(std::string msg = "Unauthorized") {
        return ApiResponse<T>(false, T{}, std::move(msg), 401);
    }

    static ApiResponse<T> forbidden(std::string msg = "Forbidden") {
        return ApiResponse<T>(false, T{}, std::move(msg), 403);
    }

    static ApiResponse<T> serverError(std::string msg = "Internal server error") {
        return ApiResponse<T>(false, T{}, std::move(msg), 500);
    }

    // ── Serialize to JSON string ──────────────
    std::string toJson() const {
        json j;
        j["success"] = success;
        j["message"] = message;
        j["statusCode"] = statusCode;

        // data chỉ include khi success = true
        if (success) {
            j["data"] = data;
        } else {
            j["data"] = nullptr;
        }
        return j.dump(2);
    }
};

// Specialization cho void (chỉ trả message)
template <>
class ApiResponse<void> {
public:
    bool        success;
    std::string message;
    int         statusCode;

    ApiResponse(bool ok, std::string msg, int code)
        : success(ok), message(std::move(msg)), statusCode(code) {}

    static ApiResponse<void> ok(std::string msg = "OK") {
        return ApiResponse<void>(true, std::move(msg), 200);
    }

    static ApiResponse<void> error(std::string msg, int code = 400) {
        return ApiResponse<void>(false, std::move(msg), code);
    }

    std::string toJson() const {
        json j;
        j["success"]    = success;
        j["message"]    = message;
        j["statusCode"] = statusCode;
        j["data"]       = nullptr;
        return j.dump(2);
    }
};
