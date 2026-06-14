// types/index.ts

export type UserRole = 'doctor' | 'pharma' | 'admin' | 'patient'

export interface User {
  id: string
  email: string
  role: UserRole
  full_name?: string
  created_at: string
}

// ── Appointment & Queue ─────────────────────────────────────

export type AppointmentStatus = 'pending' | 'confirmed' | 'cancelled' | 'done'

export interface Appointment {
  id: string
  patient_id: string
  doctor_id: string | null
  date: string           // ISO date 'YYYY-MM-DD'
  time_slot: string      // '08:30:00'
  reason: string | null
  status: AppointmentStatus
  created_at: string
  // joined
  doctor?: Pick<User, 'id' | 'full_name'>
  patient?: Pick<User, 'id' | 'full_name'>
}

export type QueueStatus = 'waiting' | 'called' | 'done' | 'skipped'

export interface QueueNumber {
  id: string
  appointment_id: string
  queue_number: number
  date: string
  called_at: string | null
  status: QueueStatus
  created_at: string
  // joined
  appointment?: Appointment
}

// ── App Settings ────────────────────────────────────────────

export interface AppSettings {
  id: 1
  logo_url: string | null
  clinic_name: string
  updated_at: string
}

// ── Patient Records (existing) ───────────────────────────────

export interface PatientRecord {
  id: string
  patient_name: string
  date_of_birth?: string
  gender?: string
  phone?: string
  address?: string
  diagnosis?: string
  prescription?: PrescriptionItem[]
  visit_date: string
  doctor_id?: string
  created_at: string
}

export interface PrescriptionItem {
  id: string
  record_id: string
  medicine_name: string
  dosage: string
  quantity: number
  unit: string
  instructions?: string
}

export interface Service {
  id: string
  name: string
  price: number
  category?: string
  description?: string
}

// ── Finance (existing) ───────────────────────────────────────

export interface Invoice {
  id: string
  patient_record_id?: string
  total_amount: number
  paid_amount: number
  status: 'unpaid' | 'partial' | 'paid'
  created_at: string
}

export interface InventoryItem {
  id: string
  name: string
  quantity: number
  unit: string
  min_quantity: number
  price_per_unit: number
  updated_at: string
}
