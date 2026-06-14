# PhòngKhám — C++ Backend

HTTP server viết bằng C++17, giao tiếp với Supabase REST API. Dùng làm lớp middleware giữa Next.js frontend và Supabase.

---

## Cấu trúc

```
backend/
├── src/
│   ├── main.cpp                  ← Entry point, load config, start server
│   ├── models/
│   │   ├── User.hpp              ← Base class cho tất cả tài khoản
│   │   ├── Doctor.hpp            ← Extends User, thêm lịch làm việc
│   │   ├── Pharmacist.hpp        ← Extends User, quyền kho/toa thuốc
│   │   ├── Admin.hpp             ← Extends User, kế thừa toàn bộ quyền
│   │   ├── PatientRecord.hpp     ← Hồ sơ một lần khám (core domain)
│   │   ├── PrescriptionItem.hpp  ← Một dòng thuốc trong toa
│   │   └── Service.hpp           ← Dịch vụ khám (khám, xét nghiệm…)
│   ├── api/
│   │   ├── SupabaseClient.hpp/cpp ← HTTP client gọi Supabase REST
│   │   └── HttpServer.hpp/cpp     ← Routes + handlers
│   └── utils/
│       ├── AuthManager.hpp/cpp   ← JWT validation, role checking
│       └── ApiResponse.hpp       ← Template response wrapper
├── libs/
│   ├── httplib.h                 ← cpp-httplib (header-only)
│   └── json.hpp                  ← nlohmann/json (header-only)
├── CMakeLists.txt
├── .env.example
└── README.md
```

---

## Yêu cầu

- C++17 compiler (g++ ≥ 9 hoặc clang++ ≥ 10)
- CMake ≥ 3.16
- OpenSSL (libssl-dev)
- cpp-httplib + nlohmann/json (đã có trong `libs/`)

```bash
# Ubuntu/Debian
sudo apt install cmake libssl-dev g++

# macOS
brew install cmake openssl
```

---

## Tải thư viện header-only

```bash
# nlohmann/json
curl -L https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp \
     -o libs/json.hpp

# cpp-httplib
curl -L https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h \
     -o libs/httplib.h
```

---

## Build

```bash
cd clinic-app/backend

# Cấu hình
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --parallel

# Binary output
./build/bin/clinic_backend
```

---

## Cấu hình

```bash
cp .env.example .env
# Chỉnh sửa .env với thông tin Supabase thực
```

Các biến bắt buộc:

| Biến | Mô tả | Lấy từ |
|------|-------|--------|
| `SUPABASE_URL` | Project URL | Dashboard → Settings → API |
| `SUPABASE_SERVICE_ROLE_KEY` | Service role key | Dashboard → Settings → API |
| `SUPABASE_JWT_SECRET` | JWT secret | Dashboard → Settings → API |
| `PORT` | Port server (mặc định 8080) | Tuỳ chọn |

---

## API Routes

### Public
```
GET  /health
```

### Xác thực (yêu cầu Bearer token)
```
GET  /api/auth/me
```

### Patient Records
```
GET    /api/records/queue          ← pharma | admin
GET    /api/records/search?name=…  ← doctor | admin
GET    /api/records                ← doctor (own) | admin (all)
GET    /api/records/:id            ← doctor | pharma | admin
POST   /api/records                ← doctor | admin
PATCH  /api/records/:id/complete   ← pharma | admin
PATCH  /api/records/:id/cancel     ← doctor | admin
```

### Users
```
GET    /api/users?role=doctor      ← admin
GET    /api/users/:id              ← admin
POST   /api/users                  ← admin
```

### Finance
```
GET    /api/finance/expenses?month=2024-06   ← admin
POST   /api/finance/expenses                 ← admin
GET    /api/finance/inventory?month=2024-06  ← admin | pharma
POST   /api/finance/inventory                ← admin | pharma
GET    /api/finance/summary?month=2024-06    ← admin
```

---

## Phân quyền

| Role | Records | Queue | Finance | Users |
|------|---------|-------|---------|-------|
| `patient` | own (tương lai) | view | ✗ | ✗ |
| `doctor` | own | ✗ | ✗ | ✗ |
| `pharma` | view all | ✓ | inventory | ✗ |
| `admin` | all | ✓ | all | ✓ |

Admin kế thừa toàn bộ quyền của doctor + pharma.

---

## Response format

Mọi response đều theo chuẩn `ApiResponse<T>`:

```json
{
  "success": true,
  "statusCode": 200,
  "message": "OK",
  "data": { ... }
}
```

Error:
```json
{
  "success": false,
  "statusCode": 401,
  "message": "Unauthorized",
  "data": null
}
```

---

## Tích hợp với Next.js frontend

Frontend hiện tại gọi Supabase trực tiếp qua `supabase-js`. Sau khi có C++ backend, frontend có thể chuyển dần sang gọi C++ API thay thế:

```typescript
// Trước: gọi Supabase trực tiếp
const { data } = await supabase.from('patient_records').select('*')

// Sau: gọi C++ backend
const res = await fetch('http://localhost:8080/api/records', {
  headers: { Authorization: `Bearer ${session.access_token}` }
})
```
