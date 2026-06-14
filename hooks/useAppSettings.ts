// hooks/useAppSettings.ts
'use client'
import { useEffect, useState } from 'react'
import { getAppSettings } from '@/lib/supabase'
import { subscribeToAppSettings, unsubscribe } from '@/lib/realtime'
import type { AppSettings } from '@/types'

export function useAppSettings() {
  const [settings, setSettings] = useState<AppSettings | null>(null)
  const [loading, setLoading]   = useState(true)

  useEffect(() => {
    let mounted = true

    getAppSettings().then(data => {
      if (mounted) { setSettings(data); setLoading(false) }
    })

    const channel = subscribeToAppSettings((payload: any) => {
      if (payload.new) setSettings(payload.new as AppSettings)
    })

    return () => {
      mounted = false
      unsubscribe(channel)
    }
  }, [])

  return {
    logoUrl:    settings?.logo_url ?? null,
    clinicName: settings?.clinic_name ?? 'PhòngKhám',
    settings,
    loading,
  }
}
