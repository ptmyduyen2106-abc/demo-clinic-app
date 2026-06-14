#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <unordered_map>
#include <csignal>

#include "api/SupabaseClient.hpp"
#include "api/HttpServer.hpp"
#include "utils/AuthManager.hpp"

// ─────────────────────────────────────────────
// loadEnv — đọc .env file đơn giản
// Format: KEY=VALUE (bỏ qua dòng bắt đầu bằng #)
// ─────────────────────────────────────────────
static std::unordered_map<std::string, std::string>
loadEnv(const std::string& path = ".env") {
    std::unordered_map<std::string, std::string> env;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[main] Warning: .env not found at '" << path << "', "
                  << "using system environment\n";
        return env;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos)        continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // Strip inline comments
        auto hash = val.find(" #");
        if (hash != std::string::npos) val = val.substr(0, hash);
        // Strip quotes
        if (val.size() >= 2 &&
            ((val.front() == '"' && val.back() == '"') ||
             (val.front() == '\'' && val.back() == '\'')))
            val = val.substr(1, val.size() - 2);
        env[key] = val;
    }
    return env;
}

static std::string getEnv(const std::unordered_map<std::string,std::string>& env,
                           const std::string& key,
                           const std::string& fallback = "") {
    // .env file overrides system env
    auto it = env.find(key);
    if (it != env.end() && !it->second.empty()) return it->second;
    // Fallback to system environment
    const char* sys = std::getenv(key.c_str());
    if (sys && *sys) return std::string(sys);
    return fallback;
}

// ─────────────────────────────────────────────
// main
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════╗\n"
              << "║  PhòngKhám — C++ Backend v1.0    ║\n"
              << "╚══════════════════════════════════╝\n\n";

    // Xác định .env path từ arg hoặc mặc định
    std::string envPath = ".env";
    if (argc > 1) envPath = argv[1];

    auto env = loadEnv(envPath);

    // ── Load config ───────────────────────────
    std::string supabaseUrl     = getEnv(env, "SUPABASE_URL");
    std::string serviceRoleKey  = getEnv(env, "SUPABASE_SERVICE_ROLE_KEY");
    std::string jwtSecret       = getEnv(env, "SUPABASE_JWT_SECRET");
    std::string portStr         = getEnv(env, "PORT", "8080");

    // Validate required config
    bool configOk = true;
    if (supabaseUrl.empty()) {
        std::cerr << "[main] ERROR: SUPABASE_URL is required\n";
        configOk = false;
    }
    if (serviceRoleKey.empty()) {
        std::cerr << "[main] ERROR: SUPABASE_SERVICE_ROLE_KEY is required\n";
        configOk = false;
    }
    if (jwtSecret.empty()) {
        std::cerr << "[main] ERROR: SUPABASE_JWT_SECRET is required\n";
        configOk = false;
    }
    if (!configOk) return 1;

    int port = 8080;
    try { port = std::stoi(portStr); }
    catch (...) { std::cerr << "[main] Invalid PORT, using 8080\n"; }

    std::cout << "[main] Supabase URL : " << supabaseUrl << "\n"
              << "[main] Port        : " << port << "\n\n";

    // ── Bootstrap services ────────────────────
    auto db   = std::make_shared<SupabaseClient>(supabaseUrl, serviceRoleKey);
    auto auth = std::make_shared<AuthManager>(jwtSecret);

    // ── Start HTTP server ─────────────────────
    HttpServer server(port, db, auth);

    // Graceful shutdown on SIGINT / SIGTERM
    std::signal(SIGINT,  [](int){ std::cout << "\n[main] Shutting down…\n"; std::exit(0); });
    std::signal(SIGTERM, [](int){ std::cout << "\n[main] Shutting down…\n"; std::exit(0); });

    server.start();   // blocking

    return 0;
}
