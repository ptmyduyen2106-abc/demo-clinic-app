// components/NavBar.tsx
'use client'
import Link from 'next/link'
import Image from 'next/image'
import { useRouter } from 'next/navigation'
import { useAuth } from '@/contexts/AuthContext'
import { useAppSettings } from '@/hooks/useAppSettings'
import { signOut } from '@/lib/supabase'

export default function NavBar() {
  const { user } = useAuth()
  const { logoUrl, clinicName, loading } = useAppSettings()
  const router = useRouter()

  async function handleSignOut() {
    await signOut()
    router.push('/login')
  }

  const navLinks: Record<string, { href: string; label: string }[]> = {
    doctor: [
      { href: '/doctor',   label: 'Khám bệnh' },
    ],
    pharma: [
      { href: '/pharmacy', label: 'Nhà thuốc' },
    ],
    finance: [
      { href: '/finance',  label: 'Tài chính' },
    ],
    admin: [
      { href: '/doctor',         label: 'Khám bệnh' },
      { href: '/pharmacy',       label: 'Nhà thuốc' },
      { href: '/finance',        label: 'Tài chính' },
      { href: '/admin/settings', label: '⚙ Cài đặt' },
    ],
    patient: [
      { href: '/patient/booking', label: 'Đặt lịch' },
      { href: '/patient/queue',   label: 'Số thứ tự' },
    ],
  }

  const links = user?.role ? (navLinks[user.role] ?? []) : []

  return (
    <nav className="navbar">
      <Link href="/" className="navbar-brand">
        {!loading && logoUrl ? (
          <Image
            src={logoUrl}
            alt={clinicName}
            width={36}
            height={36}
            className="navbar-logo"
            unoptimized
          />
        ) : (
          <span className="navbar-logo-fallback">🏥</span>
        )}
        <span className="navbar-name">{clinicName}</span>
      </Link>

      <div className="navbar-links">
        {links.map(l => (
          <Link key={l.href} href={l.href} className="nav-link">{l.label}</Link>
        ))}
      </div>

      <div className="navbar-right">
        {user && (
          <>
            <span className="navbar-user">
              {user.full_name ?? user.email}
              <span className="role-badge">{user.role}</span>
            </span>
            <button className="btn btn-ghost btn-sm" onClick={handleSignOut}>
              Đăng xuất
            </button>
          </>
        )}
      </div>

      <style jsx>{`
        .navbar {
          display: flex;
          align-items: center;
          gap: 1.5rem;
          padding: 0 1.5rem;
          height: 56px;
          background: var(--bg2);
          border-bottom: 1px solid var(--border);
          position: sticky;
          top: 0;
          z-index: 50;
        }
        .navbar-brand {
          display: flex; align-items: center; gap: 0.5rem;
          text-decoration: none; color: inherit; font-weight: 700;
          white-space: nowrap;
        }
        .navbar-logo { border-radius: 6px; object-fit: contain; }
        .navbar-logo-fallback { font-size: 1.4rem; }
        .navbar-name { font-size: 1rem; }
        .navbar-links {
          display: flex; align-items: center; gap: 0.25rem; flex: 1;
        }
        .nav-link {
          padding: 0.35rem 0.75rem;
          border-radius: 6px;
          text-decoration: none;
          color: inherit;
          font-size: 0.875rem;
          opacity: 0.75;
          transition: opacity 0.15s, background 0.15s;
        }
        .nav-link:hover { opacity: 1; background: var(--bg3, rgba(0,0,0,0.05)); }
        .navbar-right {
          display: flex; align-items: center; gap: 0.75rem;
          margin-left: auto;
        }
        .navbar-user {
          display: flex; align-items: center; gap: 0.5rem;
          font-size: 0.85rem; opacity: 0.8;
        }
        .role-badge {
          padding: 0.15rem 0.5rem;
          border-radius: 999px;
          background: var(--accent, #6366f1);
          color: #fff;
          font-size: 0.7rem;
          font-weight: 600;
          text-transform: uppercase;
        }
        .btn-ghost {
          background: transparent; border: 1px solid var(--border);
          padding: 0.3rem 0.7rem; border-radius: 6px; cursor: pointer;
          font-size: 0.8rem; color: inherit;
        }
        .btn-ghost:hover { background: var(--bg3, rgba(0,0,0,0.06)); }
      `}</style>
    </nav>
  )
}
