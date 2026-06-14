// lib/supabase.ts
import { createClient } from '@supabase/supabase-js'
import type {
  Appointment,
  AppointmentStatus,
  QueueNumber,
  QueueStatus,
  AppSettings,
  PatientRecord,
  PrescriptionItem,
  InventoryItem,
} from '@/types'

const supabaseUrl  = process.env.NEXT_PUBLIC_SUPABASE_URL!
const supabaseAnon = process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY!

export const supabase = createClient(supabaseUrl, supabaseAnon)

// ── Auth helpers ─────────────────────────────────────────────

export async function getCurrentUser() {
  const { data: { user } } = await supabase.auth.getUser()
  if (!user) return null
  const { data } = await supabase
    .from('users')
    .select('*')
    .eq('id', user.id)
    .single()
  return data
}

export async function signOut() {
  return supabase.auth.signOut()
}

// ── Patient Records (existing) ───────────────────────────────

export async function getPatientRecords() {
  return supabase
    .from('patient_records')
    .select('*')
    .order('created_at', { ascending: false })
}

export async function createPatientRecord(record: Omit<PatientRecord, 'id' | 'created_at'>) {
  return supabase.from('patient_records').insert(record).select().single()
}

export async function updatePatientRecord(id: string, updates: Partial<PatientRecord>) {
  return supabase.from('patient_records').update(updates).eq('id', id).select().single()
}

// ── Inventory (existing) ─────────────────────────────────────

export async function getInventory() {
  return supabase.from('inventory').select('*').order('name')
}

export async function updateInventoryItem(id: string, updates: Partial<InventoryItem>) {
  return supabase.from('inventory').update(updates).eq('id', id).select().single()
}

// ── Appointments ─────────────────────────────────────────────

export async function getMyAppointments(patientId: string) {
  return supabase
    .from('appointments')
    .select(`
      *,
      doctor:doctor_id ( id, full_name )
    `)
    .eq('patient_id', patientId)
    .order('date', { ascending: true })
    .order('time_slot', { ascending: true })
}

export async function getAllAppointments(date?: string) {
  let query = supabase
    .from('appointments')
    .select(`
      *,
      doctor:doctor_id ( id, full_name ),
      patient:patient_id ( id, full_name )
    `)
    .order('date', { ascending: true })
    .order('time_slot', { ascending: true })

  if (date) query = query.eq('date', date)
  return query
}

export async function createAppointment(
  payload: Pick<Appointment, 'patient_id' | 'doctor_id' | 'date' | 'time_slot' | 'reason'>
) {
  return supabase
    .from('appointments')
    .insert(payload)
    .select()
    .single()
}

export async function updateAppointmentStatus(id: string, status: AppointmentStatus) {
  return supabase
    .from('appointments')
    .update({ status })
    .eq('id', id)
    .select()
    .single()
}

export async function cancelAppointment(id: string) {
  return updateAppointmentStatus(id, 'cancelled')
}

// ── Queue numbers ────────────────────────────────────────────

export async function getDailyQueue(date: string) {
  return supabase
    .from('queue_numbers')
    .select(`
      *,
      appointment:appointment_id (
        *,
        patient:patient_id ( id, full_name ),
        doctor:doctor_id ( id, full_name )
      )
    `)
    .eq('date', date)
    .order('queue_number', { ascending: true })
}

export async function getQueueByAppointment(appointmentId: string) {
  return supabase
    .from('queue_numbers')
    .select('*')
    .eq('appointment_id', appointmentId)
    .maybeSingle()
}

export async function updateQueueStatus(id: string, status: QueueStatus) {
  const update: Partial<QueueNumber> = { status }
  if (status === 'called') update.called_at = new Date().toISOString()
  return supabase
    .from('queue_numbers')
    .update(update)
    .eq('id', id)
    .select()
    .single()
}

export async function callNextQueue(date: string) {
  // Lấy số tiếp theo đang 'waiting'
  const { data: next } = await supabase
    .from('queue_numbers')
    .select('*')
    .eq('date', date)
    .eq('status', 'waiting')
    .order('queue_number', { ascending: true })
    .limit(1)
    .single()

  if (!next) return { data: null, error: new Error('Không còn số trong hàng chờ') }
  return updateQueueStatus(next.id, 'called')
}

export async function getAvailableSlots(doctorId: string, date: string): Promise<string[]> {
  const ALL_SLOTS = [
    '08:00','08:30','09:00','09:30','10:00','10:30',
    '13:30','14:00','14:30','15:00','15:30','16:00',
  ]
  const { data: booked } = await supabase
    .from('appointments')
    .select('time_slot')
    .eq('doctor_id', doctorId)
    .eq('date', date)
    .in('status', ['pending', 'confirmed'])

  const bookedSlots = new Set((booked ?? []).map(r => r.time_slot.slice(0, 5)))
  return ALL_SLOTS.filter(s => !bookedSlots.has(s))
}

// ── App Settings & Logo ───────────────────────────────────────

export async function getAppSettings(): Promise<AppSettings | null> {
  const { data } = await supabase
    .from('app_settings')
    .select('*')
    .eq('id', 1)
    .single()
  return data
}

export async function updateAppSettings(updates: Partial<Pick<AppSettings, 'logo_url' | 'clinic_name'>>) {
  return supabase
    .from('app_settings')
    .update(updates)
    .eq('id', 1)
    .select()
    .single()
}

export async function uploadLogo(file: File): Promise<string> {
  const ext  = file.name.split('.').pop() ?? 'png'
  const path = `logo/clinic-logo.${ext}`

  const { error: uploadError } = await supabase.storage
    .from('branding')
    .upload(path, file, { upsert: true, cacheControl: '3600' })

  if (uploadError) throw uploadError

  const { data } = supabase.storage.from('branding').getPublicUrl(path)
  // Bust cache bằng timestamp
  return `${data.publicUrl}?v=${Date.now()}`
}

export async function getDoctors() {
  return supabase
    .from('users')
    .select('id, full_name')
    .eq('role', 'doctor')
    .order('full_name')
}
