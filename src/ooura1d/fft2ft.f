! test of fft2f.f
!
      include 'fft2f.f'
!
      program main
      integer nmax
      real*8 pi
      parameter (nmax = 8192)
      parameter (pi = 3.14159265358979323846d0)
      integer n
      real*8 a(0 : nmax), err, errorcheck
!
      write (*, *) 'data length n=? '
      read (*, *) n
!
!   check of CDFT
      call putdata(0, n - 1, a)
      call cdft(n, cos(2 * pi / n), sin(2 * pi / n), a)
      call cdft(n, cos(2 * pi / n), -sin(2 * pi / n), a)
      err = errorcheck(0, n - 1, 2.0d0 / n, a)
      write (*, *) 'cdft err= ', err
!
!   check of RDFT
      call putdata(0, n - 1, a)
      call rdft(n, cos(pi / n), sin(pi / n), a)
      call rdft(n, cos(pi / n), -sin(pi / n), a)
      err = errorcheck(0, n - 1, 2.0d0 / n, a)
      write (*, *) 'rdft err= ', err
!
!   check of DDCT
      call putdata(0, n - 1, a)
      call ddct(n, cos(pi / (2 * n)), sin(pi / (2 * n)), a)
      call ddct(n, cos(pi / (2 * n)), -sin(pi / (2 * n)), a)
      a(0) = a(0) * 0.5d0
      err = errorcheck(0, n - 1, 2.0d0 / n, a)
      write (*, *) 'ddct err= ', err
!
!   check of DDST
      call putdata(0, n - 1, a)
      call ddst(n, cos(pi / (2 * n)), sin(pi / (2 * n)), a)
      call ddst(n, cos(pi / (2 * n)), -sin(pi / (2 * n)), a)
      a(0) = a(0) * 0.5d0
      err = errorcheck(0, n - 1, 2.0d0 / n, a)
      write (*, *) 'ddst err= ', err
!
!   check of DFCT
      call putdata(0, n, a)
      a(0) = a(0) * 0.5d0
      a(n) = a(n) * 0.5d0
      call dfct(n, cos(pi / n), sin(pi / n), a)
      a(0) = a(0) * 0.5d0
      a(n) = a(n) * 0.5d0
      call dfct(n, cos(pi / n), sin(pi / n), a)
      err = errorcheck(0, n, 2.0d0 / n, a)
      write (*, *) 'dfct err= ', err
!
!   check of DFST
      call putdata(1, n - 1, a)
      call dfst(n, cos(pi / n), sin(pi / n), a)
      call dfst(n, cos(pi / n), sin(pi / n), a)
      err = errorcheck(1, n - 1, 2.0d0 / n, a)
      write (*, *) 'dfst err= ', err
!
      end
!
!
      subroutine putdata(nini, nend, a)
      integer nini, nend, j, seed
      real*8 a(0 : *), drnd
      seed = 0
      do j = nini, nend
          a(j) = drnd(seed)
      end do
      end
!
!
      function errorcheck(nini, nend, scale, a)
      integer nini, nend, j, seed
      real*8 scale, a(0 : *), drnd, err, e, errorcheck
      err = 0
      seed = 0
      do j = nini, nend
          e = drnd(seed) - a(j) * scale
          err = max(err, abs(e))
      end do
      errorcheck = err
      end
!
!
! random number generator, 0 <= drnd < 1
      real*8 function drnd(seed)
      integer seed
      seed = mod(seed * 7141 + 54773, 259200)
      drnd = seed * (1.0d0 / 259200)
      end
!
