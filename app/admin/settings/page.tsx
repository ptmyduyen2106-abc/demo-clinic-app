// app/admin/settings/page.tsx
'use client'
import { useRef, useState } from 'react'
import Image from 'next/image'
import { useAuth } from '@/contexts/AuthContext'
import { useAppSettings } from '@/hooks/useAppSettings'
import { uploadLogo, updateAppSettings } from '@/lib/supabase'
import { useRouter } from 'next/navigation'

export default function AdminSettingsPage() {
  const { user, loading: authLoading } = useAuth()
  const { logoUrl, clinicName, loading: settingsLoading } = useAppSettings()
  const [newName, setNewName]   = useState('')
  const [preview, setPreview]   = useState<string | null>(null)
  const [file, setFile]         = useState<File | null>(null)
  const [saving, setSaving]     = useState(false)
  const [message, setMessage]   = useState<{ type: 'ok' | 'err'; text: string } | null>(null)
  const fileRef = useRef<HTMLInputElement>(null)
  const router  = useRouter()

  if (!authLoading && user?.role !== 'admin') {
    router.replace('/')
    return null
  }

  function handleFileChange(e: React.ChangeEvent<HTMLInputElement>) {
    const f = e.target.files?.[0]
    if (!f) return
    setFile(f)
    const url = URL.createObjectURL(f)
    setPreview(url)
  }

  async function handleSave() {
    setSaving(true)
    setMessage(null)
    try {
      const updates: { logo_url?: string; clinic_name?: string } = {}

      if (file) {
        const publicUrl = await uploadLogo(file)
        updates.logo_url = publicUrl
      }
      if (newName.trim() && newName.trim() !== clinicName) {
        updates.clinic_name = newName.trim()
      }

      if (Object.keys(updates).length === 0) {
        setMessage({ type: 'err', text: 'Chưa có thay đổi nào.' })
        return
      }

      const { error } = await updateAppSettings(updates)
      if (error) throw error

      setMessage({ type: 'ok', text: 'Lưu thành công!' })
      setFile(null)
      setPreview(null)
      setNewName('')
    } catch (err: any) {
      setMessage({ type: 'err', text: err.message ?? 'Lưu thất bại.' })
    } finally {
      setSaving(false)
    }
  }

  const currentLogo = preview ?? logoUrl

  return (
    <main className="settings-page">
      <h1 className="page-title">Cài đặt phòng khám</h1>

      <div className="settings-card">
        <h2 className="section-title">Logo phòng khám</h2>

        <div className="logo-row">
          <div className="logo-preview">
            {currentLogo
              ? <Image src={currentLogo} alt="Logo" width={80} height={80} style={{ borderRadius: 12, objectFit: 'contain' }} unoptimized />
              : <div className="logo-empty">🏥</div>
            }
          </div>
          <div className="logo-actions">
            <input
              ref={fileRef}
              type="file"
              accept="image/png,image/jpeg,image/webp,image/svg+xml"
              style={{ display: 'none' }}
              onChange={handleFileChange}
            />
            <button className="btn btn-secondary" onClick={() => fileRef.current?.click()}>
              {logoUrl ? 'Đổi logo' : 'Tải logo lên'}
            </button>
            <p className="hint">PNG, JPG, WebP hoặc SVG. Khuyến nghị 256×256px.</p>
            {preview && <p className="preview-note">✦ Xem trước — nhấn Lưu để áp dụng</p>}
          </div>
        </div>
      </div>

      <div className="settings-card">
        <h2 className="section-title">Tên phòng khám</h2>
        <input
          type="text"
          className="text-input"
          placeholder={clinicName}
          value={newName}
          onChange={e => setNewName(e.target.value)}
          maxLength={80}
        />
        <p className="hint">Hiển thị trên thanh điều hướng và trong các thông báo.</p>
      </div>

      {message && (
        <div className={`message ${message.type}`}>{message.text}</div>
      )}

      <div className="save-row">
        <button className="btn btn-primary" onClick={handleSave} disabled={saving || settingsLoading}>
          {saving ? 'Đang lưu...' : 'Lưu thay đổi'}
        </button>
      </div>

      <style jsx>{`
        .settings-page {
          max-width: 640px; margin: 0 auto;
          padding: 2rem 1.5rem;
          display: flex; flex-direction: column; gap: 1.5rem;
        }
        .page-title { font-size: 1.5rem; font-weight: 700; margin: 0; }
        .settings-card {
          background: var(--bg2);
          border: 1px solid var(--border);
          border-radius: 12px;
          padding: 1.5rem;
          display: flex; flex-direction: column; gap: 1rem;
        }
        .section-title { font-size: 1rem; font-weight: 600; margin: 0; }
        .logo-row { display: flex; align-items: center; gap: 1.5rem; }
        .logo-preview {
          width: 80px; height: 80px;
          border: 2px dashed var(--border);
          border-radius: 12px;
          display: flex; align-items: center; justify-content: center;
          flex-shrink: 0; overflow: hidden;
        }
        .logo-empty { font-size: 2.5rem; }
        .logo-actions { display: flex; flex-direction: column; gap: 0.4rem; }
        .hint { font-size: 0.8rem; opacity: 0.55; margin: 0; }
        .preview-note { font-size: 0.8rem; color: #6366f1; margin: 0; }
        .text-input {
          padding: 0.65rem 0.9rem;
          border: 1px solid var(--border);
          border-radius: 8px;
          background: var(--bg3, #f9fafb);
          color: var(--text);
          font-size: 0.95rem;
          width: 100%;
        }
        .message {
          padding: 0.75rem 1rem;
          border-radius: 8px;
          font-size: 0.875rem;
        }
        .message.ok  { background: #e6f9f0; color: #166534; border: 1px solid #bbf7d0; }
        .message.err { background: #fef2f2; color: #991b1b; border: 1px solid #fecaca; }
        .save-row { display: flex; justify-content: flex-end; }
        .btn-secondary {
          padding: 0.5rem 1rem; border-radius: 8px; cursor: pointer;
          border: 1px solid var(--border); background: transparent; color: inherit;
          font-size: 0.875rem;
        }
      `}</style>
    </main>
  )
}
