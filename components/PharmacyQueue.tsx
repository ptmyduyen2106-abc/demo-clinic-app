// components/PharmacyQueue.tsx
// Cập nhật: thêm tab "Hàng chờ" tích hợp StaffQueueBoard để dược sĩ biết số đang gọi
'use client'
import { useEffect, useState } from 'react'
import { StaffQueueBoard } from '@/components/QueueStatus'
import { supabase } from '@/lib/supabase'
import { subscribeToQueue, unsubscribe } from '@/lib/realtime'

const TODAY = new Date().toISOString().split('T')[0]

interface QueueRecord {
  id: string
  patient_name: string
  visit_date: string
  diagnosis?: string
  prescription?: any[]
  status?: string
}

export default function PharmacyQueue() {
  const [records, setRecords]     = useState<QueueRecord[]>([])
  const [loading, setLoading]     = useState(true)
  const [activeTab, setActiveTab] = useState<'prescriptions' | 'queue'>('queue')
  const [selected, setSelected]   = useState<QueueRecord | null>(null)

  async function loadPrescriptions() {
    setLoading(true)
    const { data } = await supabase
      .from('patient_records')
      .select('*')
      .eq('visit_date', TODAY)
      .not('prescription', 'is', null)
      .order('created_at', { ascending: true })
    setRecords((data as QueueRecord[]) ?? [])
    setLoading(false)
  }

  useEffect(() => {
    loadPrescriptions()
    const ch = subscribeToQueue(loadPrescriptions)
    return () => unsubscribe(ch)
  }, [])

  return (
    <div className="pharmacy-wrap">
      <div className="tabs">
        <button className={`tab ${activeTab === 'queue' ? 'active' : ''}`} onClick={() => setActiveTab('queue')}>
          Số hàng chờ
        </button>
        <button className={`tab ${activeTab === 'prescriptions' ? 'active' : ''}`} onClick={() => setActiveTab('prescriptions')}>
          Đơn thuốc hôm nay
          {records.length > 0 && <span className="badge">{records.length}</span>}
        </button>
      </div>

      {activeTab === 'queue' && (
        <div className="tab-content">
          <p className="hint">Dược sĩ có thể theo dõi số đang gọi để chuẩn bị đơn thuốc trước.</p>
          <StaffQueueBoard date={TODAY} />
        </div>
      )}

      {activeTab === 'prescriptions' && (
        <div className="tab-content">
          {loading ? (
            <p className="loading">Đang tải...</p>
          ) : records.length === 0 ? (
            <p className="empty">Chưa có đơn thuốc nào hôm nay.</p>
          ) : (
            <div className="rx-layout">
              <div className="rx-list">
                {records.map(r => (
                  <button
                    key={r.id}
                    className={`rx-item ${selected?.id === r.id ? 'active' : ''}`}
                    onClick={() => setSelected(r)}
                  >
                    <span className="rx-name">{r.patient_name}</span>
                    <span className="rx-count">{(r.prescription ?? []).length} thuốc</span>
                  </button>
                ))}
              </div>

              {selected && (
                <div className="rx-detail">
                  <h3 className="detail-title">{selected.patient_name}</h3>
                  {selected.diagnosis && <p className="detail-dx">Chẩn đoán: {selected.diagnosis}</p>}
                  <table className="rx-table">
                    <thead>
                      <tr><th>Thuốc</th><th>Liều</th><th>SL</th><th>ĐV</th><th>Hướng dẫn</th></tr>
                    </thead>
                    <tbody>
                      {(selected.prescription ?? []).map((item: any, i: number) => (
                        <tr key={i}>
                          <td>{item.medicine_name}</td>
                          <td>{item.dosage}</td>
                          <td>{item.quantity}</td>
                          <td>{item.unit}</td>
                          <td>{item.instructions ?? '—'}</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                  <button className="btn btn-primary btn-full" onClick={() => setSelected(null)}>
                    Đã cấp phát
                  </button>
                </div>
              )}
            </div>
          )}
        </div>
      )}

      <style jsx>{`
        .pharmacy-wrap { display: flex; flex-direction: column; gap: 0; }
        .tabs { display: flex; gap: 0.25rem; border-bottom: 1px solid var(--border); margin-bottom: 1.25rem; }
        .tab {
          padding: 0.55rem 1rem; border: none; background: none; cursor: pointer;
          font-size: 0.875rem; color: inherit; opacity: 0.6;
          border-bottom: 2px solid transparent; margin-bottom: -1px;
          display: flex; align-items: center; gap: 0.4rem;
        }
        .tab.active { opacity: 1; border-bottom-color: var(--accent, #6366f1); font-weight: 600; }
        .badge { background: var(--accent, #6366f1); color: #fff; padding: 0.1rem 0.45rem; border-radius: 999px; font-size: 0.7rem; }
        .tab-content { display: flex; flex-direction: column; gap: 1rem; }
        .hint { font-size: 0.8rem; opacity: 0.55; margin: 0; }
        .loading, .empty { opacity: 0.45; font-size: 0.875rem; }
        .rx-layout { display: grid; grid-template-columns: 200px 1fr; gap: 1rem; }
        .rx-list { display: flex; flex-direction: column; gap: 0.4rem; }
        .rx-item {
          display: flex; flex-direction: column; gap: 0.15rem;
          padding: 0.6rem 0.8rem; border-radius: 8px;
          border: 1px solid var(--border); background: var(--bg2);
          cursor: pointer; text-align: left;
        }
        .rx-item.active { background: var(--accent, #6366f1); color: #fff; border-color: transparent; }
        .rx-name { font-size: 0.875rem; font-weight: 600; }
        .rx-count { font-size: 0.75rem; opacity: 0.7; }
        .rx-detail { display: flex; flex-direction: column; gap: 0.75rem; }
        .detail-title { margin: 0; font-size: 1.1rem; font-weight: 700; }
        .detail-dx { margin: 0; font-size: 0.85rem; opacity: 0.65; }
        .rx-table { width: 100%; border-collapse: collapse; font-size: 0.85rem; }
        .rx-table th { text-align: left; padding: 0.4rem 0.5rem; border-bottom: 1px solid var(--border); opacity: 0.6; font-weight: 600; }
        .rx-table td { padding: 0.4rem 0.5rem; border-bottom: 1px solid var(--border); }
        .btn-full { width: 100%; padding: 0.65rem; border-radius: 8px; border: none; cursor: pointer; }
        .btn-primary { background: var(--accent, #6366f1); color: #fff; }
        @media (max-width: 600px) { .rx-layout { grid-template-columns: 1fr; } }
      `}</style>
    </div>
  )
}
