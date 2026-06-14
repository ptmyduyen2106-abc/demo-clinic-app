// app/patient/booking/page.tsx
'use client'
import { useState } from 'react'
import { useRouter } from 'next/navigation'
import { useAuth } from '@/contexts/AuthContext'
import { useMyAppointments } from '@/hooks/useAppointments'
import AppointmentForm from '@/components/AppointmentForm'
import type { Appointment } from '@/types'

const STATUS_LABEL: Record<Appointment['status'], string> = {
  pending:   '⏳ Chờ xác nhận',
  confirmed: '✅ Đã xác nhận',
  cancelled: '✗ Đã huỷ',
  done:      '✓ Đã khám',
}
const STATUS_COLOR: Record<Appointment['status'], string> = {
  pending:   '#f59e0b',
  confirmed: '#16a34a',
  cancelled: '#9ca3af',
  done:      '#6366f1',
}

export default function BookingPage() {
  const { user, loading: authLoading } = useAuth()
  const router = useRouter()
  const [tab, setTab] = useState<'new' | 'history'>('new')

  const { appointments, loading, book, cancel } = useMyAppointments(user?.id)

  if (!authLoading && !user) { router.replace('/login'); return null }
  if (!authLoading && user?.role !== 'patient') { router.replace('/'); return null }

  const upcoming = appointments.filter(a => a.status !== 'done' && a.status !== 'cancelled')
  const past     = appointments.filter(a => a.status === 'done' || a.status === 'cancelled')

  return (
    <main className="booking-page">
      <h1 className="page-title">Đặt lịch khám</h1>

      <div className="tabs">
        <button className={`tab ${tab === 'new' ? 'active' : ''}`} onClick={() => setTab('new')}>
          Đặt lịch mới
        </button>
        <button className={`tab ${tab === 'history' ? 'active' : ''}`} onClick={() => setTab('history')}>
          Lịch hẹn của tôi
          {upcoming.length > 0 && <span className="badge">{upcoming.length}</span>}
        </button>
      </div>

      {tab === 'new' && (
        <div className="card">
          {user && (
            <AppointmentForm
              patientId={user.id}
              onBook={async (payload) => {
                const appt = await book(payload as any)
                setTab('history')
                return appt
              }}
            />
          )}
        </div>
      )}

      {tab === 'history' && (
        <div className="history-section">
          {loading ? (
            <p className="empty">Đang tải...</p>
          ) : appointments.length === 0 ? (
            <p className="empty">Chưa có lịch hẹn nào. <button className="link-btn" onClick={() => setTab('new')}>Đặt lịch ngay</button></p>
          ) : (
            <>
              {upcoming.length > 0 && (
                <>
                  <h2 className="sub-title">Sắp tới</h2>
                  {upcoming.map(a => (
                    <AppointmentCard
                      key={a.id}
                      appt={a}
                      onCancel={() => cancel(a.id)}
                      onViewQueue={() => router.push(`/patient/queue?appointmentId=${a.id}`)}
                    />
                  ))}
                </>
              )}
              {past.length > 0 && (
                <>
                  <h2 className="sub-title">Lịch sử</h2>
                  {past.map(a => (
                    <AppointmentCard key={a.id} appt={a} />
                  ))}
                </>
              )}
            </>
          )}
        </div>
      )}

      <style jsx>{`
        .booking-page { max-width: 680px; margin: 0 auto; padding: 2rem 1.5rem; display: flex; flex-direction: column; gap: 1.5rem; }
        .page-title { font-size: 1.5rem; font-weight: 700; margin: 0; }
        .tabs { display: flex; gap: 0.25rem; border-bottom: 1px solid var(--border); }
        .tab {
          padding: 0.6rem 1rem; border: none; background: none; cursor: pointer;
          font-size: 0.9rem; color: inherit; opacity: 0.6; border-bottom: 2px solid transparent;
          margin-bottom: -1px; display: flex; align-items: center; gap: 0.4rem;
        }
        .tab.active { opacity: 1; border-bottom-color: var(--accent, #6366f1); font-weight: 600; }
        .badge { background: var(--accent, #6366f1); color: #fff; padding: 0.1rem 0.45rem; border-radius: 999px; font-size: 0.7rem; }
        .card { background: var(--bg2); border: 1px solid var(--border); border-radius: 12px; padding: 1.5rem; }
        .history-section { display: flex; flex-direction: column; gap: 1rem; }
        .sub-title { font-size: 0.85rem; font-weight: 600; opacity: 0.5; text-transform: uppercase; margin: 0; }
        .empty { opacity: 0.5; text-align: center; padding: 2rem; }
        .link-btn { background: none; border: none; color: var(--accent, #6366f1); cursor: pointer; font-size: inherit; text-decoration: underline; }
      `}</style>
    </main>
  )
}

function AppointmentCard({
  appt, onCancel, onViewQueue,
}: {
  appt: Appointment
  onCancel?: () => void
  onViewQueue?: () => void
}) {
  const [cancelling, setCancelling] = useState(false)

  async function handleCancel() {
    if (!confirm('Bạn có chắc muốn huỷ lịch hẹn này?')) return
    setCancelling(true)
    try { await onCancel?.() } finally { setCancelling(false) }
  }

  const dateStr = new Date(appt.date + 'T00:00:00').toLocaleDateString('vi-VN', {
    weekday: 'long', year: 'numeric', month: 'long', day: 'numeric'
  })

  return (
    <div className="appt-card">
      <div className="appt-header">
        <div>
          <p className="appt-date">{dateStr} · {appt.time_slot?.slice(0, 5)}</p>
          <p className="appt-doctor">Bác sĩ: {(appt as any).doctor?.full_name ?? '—'}</p>
          {appt.reason && <p className="appt-reason">Lý do: {appt.reason}</p>}
        </div>
        <span className="status-badge" style={{ background: STATUS_COLOR[appt.status] + '22', color: STATUS_COLOR[appt.status] }}>
          {STATUS_LABEL[appt.status]}
        </span>
      </div>
      <div className="appt-actions">
        {appt.status === 'confirmed' && onViewQueue && (
          <button className="btn btn-secondary btn-sm" onClick={onViewQueue}>Xem số thứ tự</button>
        )}
        {(appt.status === 'pending' || appt.status === 'confirmed') && onCancel && (
          <button className="btn btn-danger btn-sm" onClick={handleCancel} disabled={cancelling}>
            {cancelling ? 'Đang huỷ...' : 'Huỷ lịch'}
          </button>
        )}
      </div>

      <style jsx>{`
        .appt-card {
          background: var(--bg2); border: 1px solid var(--border);
          border-radius: 10px; padding: 1rem 1.25rem;
          display: flex; flex-direction: column; gap: 0.75rem;
        }
        .appt-header { display: flex; justify-content: space-between; align-items: flex-start; gap: 1rem; }
        .appt-date { font-weight: 600; margin: 0 0 0.2rem; }
        .appt-doctor, .appt-reason { margin: 0; font-size: 0.875rem; opacity: 0.7; }
        .status-badge { padding: 0.25rem 0.75rem; border-radius: 999px; font-size: 0.78rem; font-weight: 600; white-space: nowrap; }
        .appt-actions { display: flex; gap: 0.5rem; flex-wrap: wrap; }
        .btn-sm { padding: 0.35rem 0.85rem; font-size: 0.8rem; border-radius: 6px; cursor: pointer; border: none; }
        .btn-secondary { background: var(--bg3, #e5e7eb); color: inherit; }
        .btn-danger { background: #ef4444; color: #fff; }
      `}</style>
    </div>
  )
}
