// components/DoctorForm.tsx
// Phiên bản cập nhật: thêm panel "Cấp số thứ tự" và tích hợp StaffQueueBoard
'use client'
import { useState } from 'react'
import { useAuth } from '@/contexts/AuthContext'
import { StaffQueueBoard } from '@/components/QueueStatus'
import {
  createPatientRecord,
  updatePatientRecord,
  getAllAppointments,
  updateAppointmentStatus,
} from '@/lib/supabase'
import type { PatientRecord, Appointment } from '@/types'

const TODAY = new Date().toISOString().split('T')[0]

interface DoctorFormProps {
  record?: PatientRecord | null
  onSaved?: (record: PatientRecord) => void
}

export default function DoctorForm({ record, onSaved }: DoctorFormProps) {
  const { user } = useAuth()

  // Form state
  const [patientName, setPatientName]   = useState(record?.patient_name ?? '')
  const [diagnosis, setDiagnosis]       = useState(record?.diagnosis ?? '')
  const [notes, setNotes]               = useState('')
  const [saving, setSaving]             = useState(false)
  const [saveMsg, setSaveMsg]           = useState<string | null>(null)

  // Queue panel state
  const [activeTab, setActiveTab] = useState<'record' | 'queue'>('record')
  const [appointments, setAppts]  = useState<Appointment[]>([])
  const [apptLoading, setApptLoading] = useState(false)
  const [confirming, setConfirming]   = useState<string | null>(null)

  async function loadTodayAppts() {
    setApptLoading(true)
    const { data } = await getAllAppointments(TODAY)
    setAppts((data as Appointment[]) ?? [])
    setApptLoading(false)
  }

  async function handleConfirm(id: string) {
    setConfirming(id)
    try {
      await updateAppointmentStatus(id, 'confirmed')
      await loadTodayAppts()
    } finally {
      setConfirming(null)
    }
  }

  async function handleSave(e: React.FormEvent) {
    e.preventDefault()
    if (!patientName.trim()) return
    setSaving(true)
    setSaveMsg(null)
    try {
      let saved: PatientRecord
      if (record?.id) {
        const { data, error } = await updatePatientRecord(record.id, {
          patient_name: patientName,
          diagnosis,
          visit_date: TODAY,
          doctor_id: user?.id,
        })
        if (error) throw error
        saved = data as PatientRecord
      } else {
        const { data, error } = await createPatientRecord({
          patient_name: patientName,
          diagnosis,
          visit_date: TODAY,
          doctor_id: user?.id!,
        })
        if (error) throw error
        saved = data as PatientRecord
      }
      setSaveMsg('Đã lưu hồ sơ.')
      onSaved?.(saved)
    } catch (err: any) {
      setSaveMsg('Lỗi: ' + (err.message ?? 'Không thể lưu'))
    } finally {
      setSaving(false)
    }
  }

  return (
    <div className="doctor-form-wrap">
      <div className="tabs">
        <button className={`tab ${activeTab === 'record' ? 'active' : ''}`} onClick={() => setActiveTab('record')}>
          Hồ sơ bệnh nhân
        </button>
        <button
          className={`tab ${activeTab === 'queue' ? 'active' : ''}`}
          onClick={() => { setActiveTab('queue'); loadTodayAppts() }}
        >
          Hàng chờ hôm nay
        </button>
      </div>

      {activeTab === 'record' && (
        <form className="record-form" onSubmit={handleSave}>
          <div className="form-group">
            <label>Tên bệnh nhân</label>
            <input
              type="text"
              value={patientName}
              onChange={e => setPatientName(e.target.value)}
              placeholder="Họ và tên bệnh nhân"
              required
            />
          </div>
          <div className="form-group">
            <label>Chẩn đoán</label>
            <textarea
              value={diagnosis}
              onChange={e => setDiagnosis(e.target.value)}
              rows={3}
              placeholder="Ghi chẩn đoán..."
            />
          </div>
          <div className="form-group">
            <label>Ghi chú</label>
            <textarea
              value={notes}
              onChange={e => setNotes(e.target.value)}
              rows={2}
              placeholder="Ghi chú thêm..."
            />
          </div>
          {saveMsg && <p className="save-msg">{saveMsg}</p>}
          <div className="form-actions">
            <button className="btn btn-primary" type="submit" disabled={saving}>
              {saving ? 'Đang lưu...' : record ? 'Cập nhật hồ sơ' : 'Tạo hồ sơ'}
            </button>
          </div>
        </form>
      )}

      {activeTab === 'queue' && (
        <div className="queue-section">
          {/* Appointments chưa confirm → confirm để cấp số */}
          {apptLoading ? (
            <p className="loading">Đang tải lịch hẹn...</p>
          ) : (
            <>
              {appointments.filter(a => a.status === 'pending').length > 0 && (
                <div className="pending-section">
                  <h3 className="section-label">Chờ xác nhận — cấp số thứ tự</h3>
                  {appointments
                    .filter(a => a.status === 'pending')
                    .map(a => (
                      <div key={a.id} className="appt-row">
                        <div className="appt-info">
                          <span className="appt-name">{(a as any).patient?.full_name ?? '—'}</span>
                          <span className="appt-meta">{a.time_slot?.slice(0,5)} · {a.reason ?? 'Không ghi lý do'}</span>
                        </div>
                        <button
                          className="btn btn-confirm btn-sm"
                          onClick={() => handleConfirm(a.id)}
                          disabled={confirming === a.id}
                        >
                          {confirming === a.id ? '...' : '✓ Xác nhận + cấp số'}
                        </button>
                      </div>
                    ))
                  }
                </div>
              )}

              <div className="board-section">
                <h3 className="section-label">Bảng số hàng chờ hôm nay</h3>
                <StaffQueueBoard date={TODAY} />
              </div>
            </>
          )}
        </div>
      )}

      <style jsx>{`
        .doctor-form-wrap { display: flex; flex-direction: column; gap: 0; }
        .tabs { display: flex; gap: 0.25rem; border-bottom: 1px solid var(--border); margin-bottom: 1.25rem; }
        .tab {
          padding: 0.55rem 1rem; border: none; background: none; cursor: pointer;
          font-size: 0.875rem; color: inherit; opacity: 0.6;
          border-bottom: 2px solid transparent; margin-bottom: -1px;
        }
        .tab.active { opacity: 1; border-bottom-color: var(--accent, #6366f1); font-weight: 600; }
        .record-form { display: flex; flex-direction: column; gap: 1rem; }
        .form-group { display: flex; flex-direction: column; gap: 0.35rem; }
        label { font-size: 0.8rem; font-weight: 500; opacity: 0.7; }
        input[type="text"], textarea {
          padding: 0.6rem 0.8rem; border: 1px solid var(--border);
          border-radius: 8px; background: var(--bg2); color: var(--text);
          font-size: 0.9rem; width: 100%; resize: vertical;
        }
        .form-actions { display: flex; justify-content: flex-end; }
        .save-msg { font-size: 0.85rem; opacity: 0.7; margin: 0; }
        .queue-section { display: flex; flex-direction: column; gap: 1.5rem; }
        .pending-section { display: flex; flex-direction: column; gap: 0.75rem; }
        .section-label { font-size: 0.78rem; font-weight: 600; text-transform: uppercase; opacity: 0.5; margin: 0; }
        .appt-row {
          display: flex; align-items: center; justify-content: space-between; gap: 1rem;
          padding: 0.75rem 1rem; background: var(--bg2); border: 1px solid var(--border);
          border-radius: 8px;
        }
        .appt-info { display: flex; flex-direction: column; gap: 0.2rem; }
        .appt-name { font-weight: 600; font-size: 0.9rem; }
        .appt-meta { font-size: 0.8rem; opacity: 0.6; }
        .btn-confirm { background: #16a34a; color: #fff; border: none; border-radius: 6px; cursor: pointer; white-space: nowrap; }
        .btn-sm { padding: 0.4rem 0.8rem; font-size: 0.8rem; }
        .loading { opacity: 0.45; font-size: 0.875rem; }
        .board-section { display: flex; flex-direction: column; gap: 0.75rem; }
      `}</style>
    </div>
  )
}
