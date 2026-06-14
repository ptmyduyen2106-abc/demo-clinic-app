// app/patient/queue/page.tsx
'use client'
import { useSearchParams, useRouter } from 'next/navigation'
import { Suspense } from 'react'
import { useAuth } from '@/contexts/AuthContext'
import { PatientQueueStatus } from '@/components/QueueStatus'
import { useMyAppointments } from '@/hooks/useAppointments'
import Link from 'next/link'

function QueuePageInner() {
  const { user, loading: authLoading } = useAuth()
  const params          = useSearchParams()
  const router          = useRouter()
  const appointmentId   = params.get('appointmentId') ?? undefined

  const { appointments, loading } = useMyAppointments(user?.id)

  if (!authLoading && !user) { router.replace('/login'); return null }
  if (!authLoading && user?.role !== 'patient') { router.replace('/'); return null }

  // Nếu chưa có appointmentId trên URL, chọn lịch confirmed gần nhất
  const confirmed = appointments.filter(a => a.status === 'confirmed')
  const targetId  = appointmentId ?? confirmed[0]?.id

  return (
    <main className="queue-page">
      <div className="queue-header">
        <h1 className="page-title">Số thứ tự của tôi</h1>
        <Link href="/patient/booking" className="back-link">← Lịch hẹn của tôi</Link>
      </div>

      {loading ? (
        <div className="loading">Đang tải...</div>
      ) : !targetId ? (
        <div className="empty-state">
          <div className="empty-icon">📋</div>
          <h2>Chưa có lịch hẹn đã xác nhận</h2>
          <p>Số thứ tự chỉ được cấp khi phòng khám xác nhận lịch hẹn của bạn.</p>
          <Link href="/patient/booking" className="btn btn-primary">Đặt lịch ngay</Link>
        </div>
      ) : (
        <div className="queue-content">
          {confirmed.length > 1 && (
            <div className="appt-selector">
              <label>Chọn lịch hẹn:</label>
              <div className="appt-pills">
                {confirmed.map(a => (
                  <Link
                    key={a.id}
                    href={`/patient/queue?appointmentId=${a.id}`}
                    className={`pill ${a.id === targetId ? 'active' : ''}`}
                  >
                    {new Date(a.date + 'T00:00:00').toLocaleDateString('vi-VN', { day: '2-digit', month: '2-digit' })}
                    · {a.time_slot?.slice(0, 5)}
                  </Link>
                ))}
              </div>
            </div>
          )}

          <PatientQueueStatus appointmentId={targetId} />

          <div className="info-card">
            <h3>Hướng dẫn</h3>
            <ul>
              <li>Trang này tự động cập nhật — không cần tải lại.</li>
              <li>Khi đến lượt, bạn sẽ thấy thông báo màu xanh.</li>
              <li>Vui lòng có mặt tại phòng khám trước giờ hẹn 15 phút.</li>
              <li>Nếu không thể đến, huỷ lịch ít nhất 2 giờ trước.</li>
            </ul>
          </div>
        </div>
      )}

      <style jsx>{`
        .queue-page { max-width: 480px; margin: 0 auto; padding: 2rem 1.5rem; display: flex; flex-direction: column; gap: 1.5rem; }
        .queue-header { display: flex; justify-content: space-between; align-items: baseline; }
        .page-title { font-size: 1.5rem; font-weight: 700; margin: 0; }
        .back-link { font-size: 0.85rem; opacity: 0.6; text-decoration: none; }
        .back-link:hover { opacity: 1; }
        .loading { opacity: 0.45; padding: 2rem; text-align: center; }
        .empty-state {
          text-align: center; padding: 3rem 1.5rem;
          display: flex; flex-direction: column; align-items: center; gap: 0.75rem;
        }
        .empty-icon { font-size: 3rem; }
        .empty-state h2 { margin: 0; font-size: 1.1rem; }
        .empty-state p { opacity: 0.6; margin: 0; font-size: 0.875rem; }
        .queue-content { display: flex; flex-direction: column; gap: 1.25rem; }
        .appt-selector { display: flex; flex-direction: column; gap: 0.5rem; }
        .appt-selector label { font-size: 0.8rem; opacity: 0.6; }
        .appt-pills { display: flex; gap: 0.5rem; flex-wrap: wrap; }
        .pill {
          padding: 0.35rem 0.85rem; border-radius: 999px; font-size: 0.8rem;
          border: 1px solid var(--border); text-decoration: none; color: inherit;
          transition: background 0.15s;
        }
        .pill.active { background: var(--accent, #6366f1); color: #fff; border-color: transparent; }
        .info-card {
          background: var(--bg2); border: 1px solid var(--border);
          border-radius: 10px; padding: 1.25rem;
        }
        .info-card h3 { font-size: 0.875rem; font-weight: 600; margin: 0 0 0.6rem; opacity: 0.7; }
        .info-card ul { margin: 0; padding-left: 1.2rem; display: flex; flex-direction: column; gap: 0.3rem; }
        .info-card li { font-size: 0.825rem; opacity: 0.65; }
        .btn { display: inline-block; padding: 0.6rem 1.5rem; border-radius: 8px; text-decoration: none; font-size: 0.9rem; cursor: pointer; }
        .btn-primary { background: var(--accent, #6366f1); color: #fff; border: none; }
      `}</style>
    </main>
  )
}

export default function QueuePage() {
  return (
    <Suspense fallback={<div style={{ padding: '2rem', opacity: 0.4 }}>Đang tải...</div>}>
      <QueuePageInner />
    </Suspense>
  )
}
