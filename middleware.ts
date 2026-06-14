// middleware.ts
import { createMiddlewareClient } from '@supabase/auth-helpers-nextjs'
import { NextResponse } from 'next/server'
import type { NextRequest } from 'next/server'

const ROLE_ALLOWED: Record<string, string[]> = {
  '/doctor':          ['doctor', 'admin'],
  '/pharmacy':        ['pharma', 'admin'],
  '/finance':         ['finance', 'admin'],
  '/admin/settings':  ['admin'],
  '/patient/booking': ['patient'],
  '/patient/queue':   ['patient'],
}

export async function middleware(req: NextRequest) {
  const res  = NextResponse.next()
  const supa = createMiddlewareClient({ req, res })

  const { data: { session } } = await supa.auth.getSession()

  const { pathname } = req.nextUrl

  // Không chặn login page
  if (pathname.startsWith('/login')) return res

  // Chưa đăng nhập → redirect login
  if (!session) {
    return NextResponse.redirect(new URL('/login', req.url))
  }

  // Kiểm tra role cho các route được bảo vệ
  const routeKey = Object.keys(ROLE_ALLOWED).find(k => pathname.startsWith(k))
  if (routeKey) {
    const { data: userData } = await supa
      .from('users')
      .select('role')
      .eq('id', session.user.id)
      .single()

    const role = userData?.role ?? ''
    if (!ROLE_ALLOWED[routeKey].includes(role)) {
      // Redirect về trang phù hợp với role
      const fallback =
        role === 'patient' ? '/patient/booking' :
        role === 'doctor'  ? '/doctor' :
        role === 'pharma'  ? '/pharmacy' :
        role === 'admin'   ? '/doctor' : '/login'
      return NextResponse.redirect(new URL(fallback, req.url))
    }
  }

  return res
}

export const config = {
  matcher: [
    '/doctor/:path*',
    '/pharmacy/:path*',
    '/finance/:path*',
    '/admin/:path*',
    '/patient/:path*',
  ],
}
