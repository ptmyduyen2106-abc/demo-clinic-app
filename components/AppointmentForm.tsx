// components/AppointmentForm.tsx
'use client'
import { useEffect, useState } from 'react'
import { getDoctors, getAvailableSlots } from '@/lib/supabase'
import type { User } from '@/types'

interface Props {
  patientId: string
  onSuccess?: (appointmentId: string) => void
  onBook: (payload: {
    patient_id: string
    doctor_id:  string
    date:       string
    time_slot:  string
    reason:     string
  }) => Promise<{ id: string }>
}

export default function AppointmentForm({ patientId, onSuccess, onBook }: Props) {
  const [doctors, setDoctors]     = useState<Pick<User, 'id' | 'full_name'>[]>([])
  const [slots, setSlots]         = useState<string[]>([])
  const [doctorId, setDoctorId]   = useState('')
  const [date, setDate]           = useState('')
  const [timeSlot, setTimeSlot]   = useState('')
  const [reason, setReason]       = useState('')
  const [loading, setLoading]     = useState(false)
  const [error, setError]         = useState<string | null>(null)
  const [success, setSuccess]     = useState(false)

  // Ngày tối thiểu: ngày mai
  const minDate = new Date()
  minDate.setDate(minDate.getDate() + 1)
  const minDateStr = minDate.toISOString().split('T')[0]

  useEffect(() => {
    getDoctors().then(({ data }) => setDoctors((data as any[]) ?? []))
  }, [])

  useEffect(() => {
    if (!doctorId || !date) { setSlots([]); setTimeSlot(''); return }
    getAvailableSlots(doctorId, date).then(available => {
      setSlots(available)
      setTimeSlot('')
    })
  }, [doctorId, date])

  async function handleSubmit(e: React.FormEvent) {
    e.preventDefault()
    if (!doctorId || !date || !timeSlot) {
      setError('Vui lòng chọn đầy đủ bác sĩ, ngày và giờ.')
      return
    }
    setLoading(true)
    setError(null)
    try {
      const appt = await onBook({ patient_id: patientId, doctor_id: doctorId, date, time_slot: timeSlot, reason })
      setSuccess(true)
      onSuccess?.(appt.id)
    } catch (err: any) {
      setError(err.message ?? 'Đặt lịch thất bại. Vui lòng thử lại.')
    } finally {
      setLoading(false)
    }
  }

  if (success) {
    return (
      <div className="card success-card">
        <div className="success-icon">✓</div>
        <h2>Đặt lịch thành công!</h2>
        <p>Phòng khám sẽ xác nhận lịch hẹn của bạn sớm nhất có thể.</p>
        <button
          className="btn btn-primary"
          onClick={() => { setSuccess(false); setDoctorId(''); setDate(''); setTimeSlot(''); setReason('') }}
        >
          Đặt lịch mới
        </button>
      </div>
    )
  }

  return (
    <form className="appt-form" onSubmit={handleSubmit}>
      {error && <p className="form-error">{error}</p>}

      <div className="form-group">
        <label htmlFor="doctor">Bác sĩ</label>
        <select
          id="doctor"
          value={doctorId}
          onChange={e => setDoctorId(e.target.value)}
          required
        >
          <option value="">-- Chọn bác sĩ --</option>
          {doctors.map(d => (
            <option key={d.id} value={d.id}>{d.full_name}</option>
          ))}
        </select>
      </div>

      <div className="form-group">
        <label htmlFor="date">Ngày khám</label>
        <input
          id="date"
          type="date"
          value={date}
          min={minDateStr}
          onChange={e => setDate(e.target.value)}
          required
        />
      </div>

      <div className="form-group">
        <label htmlFor="slot">Giờ khám {doctorId && date && `(${slots.length} slot trống)`}</label>
        <select
          id="slot"
          value={timeSlot}
          onChange={e => setTimeSlot(e.target.value)}
          disabled={!doctorId || !date || slots.length === 0}
          required
        >
          <option value="">-- Chọn giờ --</option>
          {slots.map(s => <option key={s} value={s}>{s}</option>)}
        </select>
        {doctorId && date && slots.length === 0 && (
          <span className="slot-empty">Không còn slot trống ngày này</span>
        )}
      </div>

      <div className="form-group">
        <label htmlFor="reason">Lý do khám (tuỳ chọn)</label>
        <textarea
          id="reason"
          value={reason}
          onChange={e => setReason(e.target.value)}
          rows={3}
          placeholder="Mô tả triệu chứng hoặc lý do đến khám..."
        />
      </div>

      <button className="btn btn-primary btn-full" type="submit" disabled={loading}>
        {loading ? 'Đang đặt lịch...' : 'Xác nhận đặt lịch'}
      </button>

      <style jsx>{`
        .appt-form { display: flex; flex-direction: column; gap: 1.2rem; }
        .form-group { display: flex; flex-direction: column; gap: 0.4rem; }
        label { font-size: 0.85rem; font-weight: 500; opacity: 0.75; }
        select, input[type="date"], textarea {
          padding: 0.6rem 0.8rem;
          border: 1px solid var(--border);
          border-radius: 8px;
          background: var(--bg2);
          color: var(--text);
          font-size: 0.95rem;
          width: 100%;
        }
        select:disabled { opacity: 0.45; cursor: not-allowed; }
        .slot-empty { font-size: 0.8rem; color: var(--danger, #e74c3c); }
        .form-error {
          padding: 0.6rem 0.8rem;
          background: #fee;
          border: 1px solid #f99;
          border-radius: 8px;
          font-size: 0.875rem;
          color: #c00;
        }
        .success-card {
          text-align: center;
          padding: 2.5rem 2rem;
          display: flex;
          flex-direction: column;
          align-items: center;
          gap: 0.75rem;
        }
        .success-icon {
          width: 56px; height: 56px;
          border-radius: 50%;
          background: #e6f9f0;
          color: #16a34a;
          font-size: 1.75rem;
          display: flex; align-items: center; justify-content: center;
        }
        .btn-full { width: 100%; }
      `}</style>
    </form>
  )
}
