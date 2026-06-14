// lib/realtime.ts
import { supabase } from './supabase'
import type { RealtimeChannel } from '@supabase/supabase-js'

type Callback<T = unknown> = (payload: T) => void

// ── Generic helper ───────────────────────────────────────────

function subscribeTable<T = unknown>(
  channelName: string,
  table: string,
  filter: string | undefined,
  cb: Callback<T>
): RealtimeChannel {
  let channel = supabase.channel(channelName).on(
    'postgres_changes' as any,
    {
      event: '*',
      schema: 'public',
      table,
      ...(filter ? { filter } : {}),
    },
    (payload: any) => cb(payload as T)
  )
  channel.subscribe()
  return channel
}

// ── Queue ────────────────────────────────────────────────────

export function subscribeToQueueChanges(date: string, cb: Callback) {
  return subscribeTable(
    `queue-${date}`,
    'queue_numbers',
    `date=eq.${date}`,
    cb
  )
}

export function subscribeToMyQueue(appointmentId: string, cb: Callback) {
  return subscribeTable(
    `my-queue-${appointmentId}`,
    'queue_numbers',
    `appointment_id=eq.${appointmentId}`,
    cb
  )
}

// ── Appointments ─────────────────────────────────────────────

export function subscribeToNewAppointments(date: string, cb: Callback) {
  return subscribeTable(
    `appointments-${date}`,
    'appointments',
    `date=eq.${date}`,
    cb
  )
}

export function subscribeToMyAppointments(patientId: string, cb: Callback) {
  return subscribeTable(
    `my-appts-${patientId}`,
    'appointments',
    `patient_id=eq.${patientId}`,
    cb
  )
}

// ── App settings ─────────────────────────────────────────────

export function subscribeToAppSettings(cb: Callback) {
  return subscribeTable('app-settings', 'app_settings', undefined, cb)
}

// ── Patient records (existing) ───────────────────────────────

export function subscribeToQueue(cb: Callback) {
  return subscribeTable('pharmacy-queue', 'patient_records', undefined, cb)
}

export function unsubscribe(channel: RealtimeChannel) {
  supabase.removeChannel(channel)
}
