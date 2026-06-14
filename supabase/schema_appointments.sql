-- ============================================================
-- schema_appointments.sql
-- Chạy sau schema.sql gốc
-- ============================================================

-- 0. Cho phép role 'patient' trong bảng users
alter table public.users
  drop constraint if exists users_role_check;
alter table public.users
  add constraint users_role_check
  check (role in ('doctor','pharma','admin','patient'));

-- ============================================================
-- 1. Bảng appointments
-- ============================================================
create table if not exists appointments (
  id          uuid primary key default uuid_generate_v4(),
  patient_id  uuid references auth.users(id) on delete cascade,
  doctor_id   uuid references public.users(id) on delete set null,
  date        date not null,
  time_slot   time not null,
  reason      text,
  status      text default 'pending'
              check (status in ('pending','confirmed','cancelled','done')),
  created_at  timestamptz default now()
);

alter table appointments enable row level security;

create policy "patient sees own appointments"
  on appointments for select
  using (patient_id = auth.uid());

create policy "patient can insert own appointments"
  on appointments for insert
  with check (patient_id = auth.uid());

create policy "patient can cancel own appointments"
  on appointments for update
  using (patient_id = auth.uid())
  with check (status = 'cancelled');

create policy "staff can view all appointments"
  on appointments for select
  using (
    exists (
      select 1 from public.users
      where id = auth.uid() and role in ('doctor','admin','pharma')
    )
  );

create policy "staff can update appointment status"
  on appointments for update
  using (
    exists (
      select 1 from public.users
      where id = auth.uid() and role in ('doctor','admin')
    )
  );

-- ============================================================
-- 2. Bảng queue_numbers
-- ============================================================
create table if not exists queue_numbers (
  id             uuid primary key default uuid_generate_v4(),
  appointment_id uuid references appointments(id) on delete cascade,
  queue_number   int not null,
  date           date not null,
  called_at      timestamptz,
  status         text default 'waiting'
                 check (status in ('waiting','called','done','skipped')),
  created_at     timestamptz default now()
);

alter table queue_numbers enable row level security;

create policy "anyone authenticated can read queue"
  on queue_numbers for select
  using (auth.uid() is not null);

create policy "staff can update queue"
  on queue_numbers for update
  using (
    exists (
      select 1 from public.users
      where id = auth.uid() and role in ('doctor','admin','pharma')
    )
  );

create policy "system can insert queue numbers"
  on queue_numbers for insert
  with check (auth.uid() is not null);

-- Auto-generate queue number mỗi ngày
create or replace function assign_queue_number()
returns trigger language plpgsql as $$
declare
  next_num int;
begin
  select coalesce(max(queue_number), 0) + 1
    into next_num
    from queue_numbers
    where date = new.date;

  insert into queue_numbers (appointment_id, queue_number, date)
  values (new.id, next_num, new.date);

  return new;
end;
$$;

drop trigger if exists on_appointment_confirmed on appointments;
create trigger on_appointment_confirmed
  after update on appointments
  for each row
  when (old.status <> 'confirmed' and new.status = 'confirmed')
  execute function assign_queue_number();

-- ============================================================
-- 3. Bucket branding (public)
-- ============================================================
insert into storage.buckets (id, name, public)
values ('branding', 'branding', true)
on conflict (id) do nothing;

create policy "public read branding"
  on storage.objects for select
  using (bucket_id = 'branding');

create policy "admin can upload branding"
  on storage.objects for insert
  with check (
    bucket_id = 'branding' and
    exists (
      select 1 from public.users
      where id = auth.uid() and role = 'admin'
    )
  );

create policy "admin can update branding"
  on storage.objects for update
  using (
    bucket_id = 'branding' and
    exists (
      select 1 from public.users
      where id = auth.uid() and role = 'admin'
    )
  );

-- ============================================================
-- 4. Bảng app_settings
-- ============================================================
create table if not exists app_settings (
  id           int primary key default 1,
  logo_url     text,
  clinic_name  text not null default 'PhòngKhám',
  updated_at   timestamptz default now(),
  check (id = 1)
);

insert into app_settings (id, logo_url, clinic_name)
values (1, null, 'PhòngKhám')
on conflict (id) do nothing;

alter table app_settings enable row level security;

create policy "anyone can read settings"
  on app_settings for select
  using (true);

create policy "admin can update settings"
  on app_settings for update
  using (
    exists (
      select 1 from public.users
      where id = auth.uid() and role = 'admin'
    )
  );

-- Auto-update updated_at
create or replace function touch_app_settings()
returns trigger language plpgsql as $$
begin
  new.updated_at = now();
  return new;
end;
$$;

drop trigger if exists on_app_settings_update on app_settings;
create trigger on_app_settings_update
  before update on app_settings
  for each row execute function touch_app_settings();
