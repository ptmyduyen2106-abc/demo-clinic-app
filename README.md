# 🏥 PhòngKhám — Hệ thống Quản lý Phòng Khám

Ứng dụng quản lý phòng khám gồm 3 module: **Bác sĩ · Nhà thuốc · Tài chính** với real-time sync qua Supabase.

---

## 📁 Cấu trúc dự án

```
clinic-app/
├── app/
│   ├── (auth)/login/page.tsx     ← Trang đăng nhập
│   ├── doctor/page.tsx           ← Module Bác sĩ
│   ├── pharmacy/page.tsx         ← Module Nhà thuốc
│   ├── finance/page.tsx          ← Module Tài chính
│   ├── page.tsx                  ← Root redirect theo role
│   ├── layout.tsx                ← Root layout + font
│   └── globals.css               ← Styles + utility classes
├── components/
│   ├── NavBar.tsx                ← Navigation (role-aware)
│   ├── DoctorForm.tsx            ← Form khám + kê đơn đầy đủ
│   ├── RxBuilder.tsx             ← Kê đơn thuốc (drag/add/remove)
│   ├── PharmacyQueue.tsx         ← Hàng chờ live
│   ├── QueueCard.tsx             ← Thẻ bệnh nhân có thể mở rộng
│   ├── FinanceDashboard.tsx      ← Dashboard 4 tab
│   └── InventoryModal.tsx        ← Modal nhập phiếu hàng
├── contexts/
│   └── AuthContext.tsx           ← useAuth hook + provider
├── hooks/
│   ├── useQueue.ts               ← Real-time queue state
│   └── useFinance.ts             ← Finance data + realtime
├── lib/
│   ├── supabase.ts               ← Supabase client + DB helpers
│   └── realtime.ts               ← Subscription helpers
├── types/
│   └── index.ts                  ← TypeScript types
├── supabase/
│   └── schema.sql                ← Toàn bộ DB schema + RLS
├── middleware.ts                 ← Route guard theo role
├── .env.local.example            ← Mẫu env vars
└── README.md
```

---

## 🚀 Setup từ đầu (5 bước)

### Bước 1 — Tạo Next.js project

```bash
npx create-next-app@latest clinic-app \
  --typescript --tailwind --eslint --app \
  --src-dir=false --import-alias="@/*"
cd clinic-app
```

### Bước 2 — Cài dependencies

```bash
npm install @supabase/supabase-js @supabase/auth-helpers-nextjs
```

### Bước 3 — Copy code

Copy tất cả các file trong repo này vào project (ghi đè các file mặc định).

### Bước 4 — Setup Supabase

1. Tạo project mới tại [supabase.com](https://supabase.com)
2. Vào **SQL Editor** → paste toàn bộ nội dung `supabase/schema.sql` → **Run**
3. Tạo 3 tài khoản demo tại **Authentication → Users → Add user**:
   - `doctor@demo.vn` / `123456`
   - `pharma@demo.vn` / `123456`
   - `admin@demo.vn` / `123456`
4. Sau khi tạo, copy UUID của từng user và chạy đoạn SQL seed cuối file `schema.sql`

### Bước 5 — Env vars

```bash
cp .env.local.example .env.local
```

Điền vào `.env.local`:
```
NEXT_PUBLIC_SUPABASE_URL=https://xxxx.supabase.co
NEXT_PUBLIC_SUPABASE_ANON_KEY=eyJ...
```

Lấy từ: **Supabase Dashboard → Project Settings → API**

### Chạy app

```bash
npm run dev
```

Mở [http://localhost:3000](http://localhost:3000)

---

## 🔐 Phân quyền

| Role   | `/doctor` | `/pharmacy` | `/finance` |
|--------|-----------|-------------|------------|
| doctor | ✅        | ❌          | ❌         |
| pharma | ❌        | ✅          | ❌         |
| admin  | ✅        | ✅          | ✅         |

---

## ⚡ Real-time Flow

```
Bác sĩ điền form → insertPatientRecord()
       ↓
  Supabase INSERT event
       ↓
  subscribeToNewPatients() fires
       ↓
  Nhà thuốc thấy ngay không cần F5
       ↓
  Dược sĩ bấm "Đã giao thuốc"
       ↓
  completePatientRecord() → status: 'done'
       ↓
  subscribeToAllPatientChanges() fires
       ↓
  Finance dashboard cập nhật doanh thu
```

---

## 🗄️ Database Tables

| Table             | Mô tả                              |
|-------------------|------------------------------------|
| `users`           | Tài khoản + role                   |
| `patient_records` | Hồ sơ khám + toa thuốc + dịch vụ  |
| `expenses`        | Chi phí vận hành                   |
| `inventory`       | Phiếu nhập hàng / thuốc            |

---

## 🚀 Deploy lên Vercel

```bash
npx vercel --prod
```

Thêm env vars trong Vercel Dashboard → Project Settings → Environment Variables.

---

## 📝 Ghi chú kỹ thuật

- **`prescription`** và **`services`** lưu dưới dạng `jsonb` trong PostgreSQL — linh hoạt, không cần join table
- **Row Level Security (RLS)** bật cho tất cả tables — dữ liệu an toàn ở mức database
- **Realtime** dùng Supabase's `postgres_changes` — WebSocket tự động, 0 config phía server
- **Middleware** chạy ở Edge Runtime — route guard không tốn server resources
