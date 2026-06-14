cmake_minimum_required(VERSION 3.16)
project(clinic_backend VERSION 1.0.0 LANGUAGES CXX)

# ── C++ standard ──────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build type ────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler  : ${CMAKE_CXX_COMPILER_ID}")

# ── Compiler flags ────────────────────────────
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wall -Wextra -Wpedantic")

# ── Source files ──────────────────────────────
set(SOURCES
    src/main.cpp
    src/api/SupabaseClient.cpp
    src/api/HttpServer.cpp
    src/utils/AuthManager.cpp
)

add_executable(clinic_backend ${SOURCES})

# ── Include dirs ──────────────────────────────
target_include_directories(clinic_backend PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs
)

# ── OpenSSL (required cho HTTPS và JWT) ───────
find_package(OpenSSL REQUIRED)
target_link_libraries(clinic_backend PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

# ── cpp-httplib dùng HTTPS → cần OpenSSL ──────
target_compile_definitions(clinic_backend PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)

# ── Threads ───────────────────────────────────
find_package(Threads REQUIRED)
target_link_libraries(clinic_backend PRIVATE Threads::Threads)

# ── Output binary ─────────────────────────────
set_target_properties(clinic_backend PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "")
message(STATUS "  clinic_backend configured.")
message(STATUS "  Build: cmake --build build/")
message(STATUS "  Run  : ./build/bin/clinic_backend")
message(STATUS "")
