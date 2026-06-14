// app/page.tsx
'use client'
import { useEffect } from 'react'
import { useRouter } from 'next/navigation'
import { useAuth } from '@/contexts/AuthContext'

const ROLE_REDIRECT: Record<string, string> = {
  doctor:  '/doctor',
  pharma:  '/pharmacy',
  admin:   '/doctor',
  patient: '/patient/booking',
}

export default function RootPage() {
  const { user, loading } = useAuth()
  const router = useRouter()

  useEffect(() => {
    if (loading) return
    if (!user) { router.replace('/login'); return }
    const dest = ROLE_REDIRECT[user.role] ?? '/login'
    router.replace(dest)
  }, [user, loading, router])

  return (
    <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', minHeight: '60vh', opacity: 0.4 }}>
      Đang chuyển hướng...
    </div>
  )
}
