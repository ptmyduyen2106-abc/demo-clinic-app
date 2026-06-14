// components/QueueStatus.tsx
'use client'
import { useMyQueueNumber, useDailyQueue } from '@/hooks/useAppointments'
import type { QueueNumber } from '@/types'

// ── Patient view: số của mình ────────────────────────────────

interface PatientQueueProps {
  appointmentId: string
}

export function PatientQueueStatus({ appointmentId }: PatientQueueProps) {
  const { queue, loading } = useMyQueueNumber(appointmentId)

  if (loading) return <div className="queue-skeleton">Đang tải...</div>
  if (!queue)  return (
    <div className="queue-card pending">
      <p className="queue-label">Lịch hẹn chưa được xác nhận</p>
      <p className="queue-sub">Số thứ tự sẽ được cấp khi phòng khám xác nhận lịch của bạn.</p>
    </div>
  )

  const statusMap: Record<QueueNumber['status'], { label: string; color: string }> = {
    waiting: { label: 'Đang chờ',   color: '#f59e0b' },
    called:  { label: 'Đến lượt!',  color: '#16a34a' },
    done:    { label: 'Đã khám',    color: '#6b7280' },
    skipped: { label: 'Bỏ qua',     color: '#ef4444' },
  }
  const st = statusMap[queue.status]

  return (
    <div className="queue-card" style={{ borderColor: st.color }}>
      <p className="queue-label">Số thứ tự của bạn</p>
      <div className="queue-number" style={{ color: st.color }}>{queue.queue_number}</div>
      <span className="queue-badge" style={{ background: st.color }}>{st.label}</span>
      {queue.status === 'called' && (
        <p className="queue-alert">📢 Đến lượt bạn! Vui lòng vào phòng khám.</p>
      )}
      {queue.status === 'waiting' && (
        <p className="queue-sub">Trang này tự động cập nhật khi đến lượt bạn.</p>
      )}
      {queue.called_at && (
        <p className="queue-sub">
          Gọi lúc {new Date(queue.called_at).toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit' })}
        </p>
      )}

      <style jsx>{`
        .queue-card {
          border: 2px solid var(--border);
          border-radius: 16px;
          padding: 2rem;
          text-align: center;
          display: flex; flex-direction: column; align-items: center; gap: 0.75rem;
          transition: border-color 0.3s;
        }
        .queue-label { font-size: 0.85rem; opacity: 0.65; margin: 0; }
        .queue-number { font-size: 5rem; font-weight: 700; line-height: 1; }
        .queue-badge {
          padding: 0.25rem 1rem;
          border-radius: 999px;
          color: #fff;
          font-size: 0.8rem;
          font-weight: 600;
        }
        .queue-alert { font-weight: 600; color: #16a34a; margin: 0; }
        .queue-sub { font-size: 0.8rem; opacity: 0.6; margin: 0; }
        .queue-skeleton { opacity: 0.4; padding: 2rem; text-align: center; }
        .pending { border-color: var(--border); }
      `}</style>
    </div>
  )
}

// ── Staff view: toàn bộ hàng chờ ngày hôm nay ───────────────

interface StaffQueueProps {
  date: string
}

export function StaffQueueBoard({ date }: StaffQueueProps) {
  const { waiting, current, done, loading, callNext, setStatus } = useDailyQueue(date)

  if (loading) return <div style={{ opacity: 0.4, padding: '1rem' }}>Đang tải hàng chờ...</div>

  return (
    <div className="queue-board">
      <div className="board-header">
        <div className="current-section">
          <p className="section-label">Đang gọi</p>
          {current ? (
            <div className="current-number">
              <span className="big-num">{current.appointment?.patient?.full_name ?? `#${current.queue_number}`}</span>
              <span className="num-badge">Số {current.queue_number}</span>
              <div className="action-row">
                <button className="btn btn-sm btn-success" onClick={() => setStatus(current.id, 'done')}>✓ Xong</button>
                <button className="btn btn-sm btn-danger"  onClick={() => setStatus(current.id, 'skipped')}>✗ Bỏ qua</button>
              </div>
            </div>
          ) : (
            <p className="empty-msg">Chưa có số nào được gọi</p>
          )}
          <button className="btn btn-primary btn-call" onClick={callNext} disabled={waiting.length === 0}>
            Gọi số tiếp theo {waiting.length > 0 ? `(còn ${waiting.length})` : ''}
          </button>
        </div>

        <div className="waiting-section">
          <p className="section-label">Hàng chờ ({waiting.length})</p>
          {waiting.length === 0
            ? <p className="empty-msg">Không còn ai chờ</p>
            : waiting.map(q => (
              <div key={q.id} className="queue-row">
                <span className="q-num">#{q.queue_number}</span>
                <span className="q-name">{q.appointment?.patient?.full_name ?? '—'}</span>
                <span className="q-time">{q.appointment?.time_slot?.slice(0,5) ?? ''}</span>
              </div>
            ))
          }
        </div>
      </div>

      {done.length > 0 && (
        <div className="done-section">
          <p className="section-label">Đã khám ({done.length})</p>
          <div className="done-list">
            {done.map(q => (
              <span key={q.id} className="done-tag">
                #{q.queue_number} {q.appointment?.patient?.full_name ?? ''}
              </span>
            ))}
          </div>
        </div>
      )}

      <style jsx>{`
        .queue-board { display: flex; flex-direction: column; gap: 1.5rem; }
        .board-header { display: grid; grid-template-columns: 1fr 1fr; gap: 1.5rem; }
        .section-label { font-size: 0.78rem; font-weight: 600; text-transform: uppercase; opacity: 0.5; margin: 0 0 0.75rem; }
        .current-section, .waiting-section {
          background: var(--bg2); border: 1px solid var(--border);
          border-radius: 12px; padding: 1.25rem;
          display: flex; flex-direction: column; gap: 0.75rem;
        }
        .current-number { display: flex; flex-direction: column; gap: 0.4rem; }
        .big-num { font-size: 1.5rem; font-weight: 700; }
        .num-badge { font-size: 0.8rem; opacity: 0.6; }
        .action-row { display: flex; gap: 0.5rem; }
        .btn-call { margin-top: auto; }
        .empty-msg { opacity: 0.45; font-size: 0.875rem; margin: 0; }
        .queue-row {
          display: flex; align-items: center; gap: 0.75rem;
          padding: 0.4rem 0; border-bottom: 1px solid var(--border);
          font-size: 0.875rem;
        }
        .queue-row:last-child { border-bottom: none; }
        .q-num { font-weight: 700; min-width: 28px; }
        .q-name { flex: 1; }
        .q-time { opacity: 0.5; font-size: 0.8rem; }
        .done-section { background: var(--bg2); border: 1px solid var(--border); border-radius: 12px; padding: 1.25rem; }
        .done-list { display: flex; flex-wrap: wrap; gap: 0.4rem; }
        .done-tag {
          padding: 0.2rem 0.6rem; border-radius: 999px;
          background: #e5e7eb; color: #374151;
          font-size: 0.8rem;
        }
        .btn-sm { padding: 0.3rem 0.8rem; font-size: 0.8rem; }
        .btn-success { background: #16a34a; color: #fff; border: none; border-radius: 6px; cursor: pointer; }
        .btn-danger  { background: #ef4444; color: #fff; border: none; border-radius: 6px; cursor: pointer; }
        @media (max-width: 600px) { .board-header { grid-template-columns: 1fr; } }
      `}</style>
    </div>
  )
}
