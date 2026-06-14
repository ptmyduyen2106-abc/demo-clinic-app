// hooks/useAppointments.ts
'use client'
import { useCallback, useEffect, useRef, useState } from 'react'
import {
  getMyAppointments,
  getDailyQueue,
  getQueueByAppointment,
  createAppointment,
  cancelAppointment,
  updateAppointmentStatus,
  updateQueueStatus,
  callNextQueue,
} from '@/lib/supabase'
import {
  subscribeToMyAppointments,
  subscribeToQueueChanges,
  subscribeToMyQueue,
  unsubscribe,
} from '@/lib/realtime'
import type { Appointment, QueueNumber } from '@/types'

// ── Patient: danh sách lịch hẹn của mình ────────────────────

export function useMyAppointments(patientId: string | undefined) {
  const [appointments, setAppointments] = useState<Appointment[]>([])
  const [loading, setLoading]           = useState(true)
  const [error, setError]               = useState<string | null>(null)

  const load = useCallback(async () => {
    if (!patientId) return
    setLoading(true)
    const { data, error: err } = await getMyAppointments(patientId)
    if (err) setError(err.message)
    else setAppointments((data as Appointment[]) ?? [])
    setLoading(false)
  }, [patientId])

  useEffect(() => {
    load()
    if (!patientId) return
    const ch = subscribeToMyAppointments(patientId, load)
    return () => unsubscribe(ch)
  }, [patientId, load])

  const book = useCallback(
    async (payload: Omit<Appointment, 'id' | 'created_at' | 'status'>) => {
      const { data, error: err } = await createAppointment({
        patient_id: payload.patient_id,
        doctor_id:  payload.doctor_id,
        date:       payload.date,
        time_slot:  payload.time_slot + ':00',
        reason:     payload.reason,
      })
      if (err) throw err
      await load()
      return data as Appointment
    },
    [load]
  )

  const cancel = useCallback(
    async (id: string) => {
      const { error: err } = await cancelAppointment(id)
      if (err) throw err
      await load()
    },
    [load]
  )

  return { appointments, loading, error, book, cancel, reload: load }
}

// ── Patient: số thứ tự của mình ─────────────────────────────

export function useMyQueueNumber(appointmentId: string | undefined) {
  const [queue, setQueue]   = useState<QueueNumber | null>(null)
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    if (!appointmentId) return
    getQueueByAppointment(appointmentId).then(({ data }) => {
      setQueue(data as QueueNumber | null)
      setLoading(false)
    })

    const ch = subscribeToMyQueue(appointmentId, (payload: any) => {
      if (payload.new) setQueue(payload.new as QueueNumber)
    })
    return () => unsubscribe(ch)
  }, [appointmentId])

  return { queue, loading }
}

// ── Staff: hàng chờ theo ngày ────────────────────────────────

export function useDailyQueue(date: string) {
  const [queue, setQueue]   = useState<QueueNumber[]>([])
  const [loading, setLoading] = useState(true)
  const dateRef             = useRef(date)
  dateRef.current           = date

  const load = useCallback(async () => {
    setLoading(true)
    const { data } = await getDailyQueue(dateRef.current)
    setQueue((data as QueueNumber[]) ?? [])
    setLoading(false)
  }, [])

  useEffect(() => {
    load()
    const ch = subscribeToQueueChanges(date, load)
    return () => unsubscribe(ch)
  }, [date, load])

  const callNext = useCallback(async () => {
    const { data, error } = await callNextQueue(dateRef.current)
    if (error) throw error
    await load()
    return data as QueueNumber
  }, [load])

  const setStatus = useCallback(
    async (queueId: string, status: QueueNumber['status']) => {
      const { error } = await updateQueueStatus(queueId, status)
      if (error) throw error
      await load()
    },
    [load]
  )

  const waiting  = queue.filter(q => q.status === 'waiting')
  const called   = queue.filter(q => q.status === 'called')
  const done     = queue.filter(q => q.status === 'done')
  const current  = called[0] ?? null

  return { queue, waiting, called, done, current, loading, callNext, setStatus, reload: load }
}

// ── Staff: update trạng thái appointment ─────────────────────

export function useAppointmentActions() {
  const confirm = useCallback(async (id: string) => {
    const { error } = await updateAppointmentStatus(id, 'confirmed')
    if (error) throw error
  }, [])

  const markDone = useCallback(async (id: string) => {
    const { error } = await updateAppointmentStatus(id, 'done')
    if (error) throw error
  }, [])

  const cancel = useCallback(async (id: string) => {
    const { error } = await cancelAppointment(id)
    if (error) throw error
  }, [])

  return { confirm, markDone, cancel }
}
