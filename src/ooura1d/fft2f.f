! Fast Fourier/Cosine/Sine Transform
!     dimension   :one
!     data length :power of 2
!     decimation  :frequency
!     radix       :2
!     data        :inplace
!     table       :not use
! subroutines
!     cdft: Complex Discrete Fourier Transform
!     rdft: Real Discrete Fourier Transform
!     ddct: Discrete Cosine Transform
!     ddst: Discrete Sine Transform
!     dfct: Cosine Transform of RDFT (Real Symmetric DFT)
!     dfst: Sine Transform of RDFT (Real Anti-symmetric DFT)
!
!
! -------- Complex DFT (Discrete Fourier Transform) --------
!     [definition]
!         <case1>
!             X(k) = sum_j=0^n-1 x(j)*exp(2*pi*i*j*k/n), 0<=k<n
!         <case2>
!             X(k) = sum_j=0^n-1 x(j)*exp(-2*pi*i*j*k/n), 0<=k<n
!         (notes: sum_j=0^n-1 is a summation from j=0 to n-1)
!     [usage]
!         <case1>
!             call cdft(2*n, cos(pi/n), sin(pi/n), a)
!         <case2>
!             call cdft(2*n, cos(pi/n), -sin(pi/n), a)
!     [parameters]
!         2*n        :data length (integer)
!                     n >= 1, n = power of 2
!         a(0:2*n-1) :input/output data (real*8)
!                     input data
!                         a(2*j) = Re(x(j)), a(2*j+1) = Im(x(j)), 0<=j<n
!                     output data
!                         a(2*k) = Re(X(k)), a(2*k+1) = Im(X(k)), 0<=k<n
!     [remark]
!         Inverse of 
!             call cdft(2*n, cos(pi/n), -sin(pi/n), a)
!         is 
!             call cdft(2*n, cos(pi/n), sin(pi/n), a)
!             do j = 0, 2 * n - 1
!                 a(j) = a(j) / n
!             end do
!         .
!
!
! -------- Real DFT / Inverse of Real DFT --------
!     [definition]
!         <case1> RDFT
!             R(k) = sum_j=0^n-1 a(j)*cos(2*pi*j*k/n), 0<=k<=n/2
!             I(k) = sum_j=0^n-1 a(j)*sin(2*pi*j*k/n), 0<k<n/2
!         <case2> IRDFT (excluding scale)
!             a(k) = R(0)/2 + R(n/2)/2 + 
!                    sum_j=1^n/2-1 R(j)*cos(2*pi*j*k/n) + 
!                    sum_j=1^n/2-1 I(j)*sin(2*pi*j*k/n), 0<=k<n
!     [usage]
!         <case1>
!             call rdft(n, cos(pi/n), sin(pi/n), a)
!         <case2>
!             call rdft(n, cos(pi/n), -sin(pi/n), a)
!     [parameters]
!         n          :data length (integer)
!                     n >= 2, n = power of 2
!         a(0:n-1)   :input/output data (real*8)
!                     <case1>
!                         output data
!                             a(2*k) = R(k), 0<=k<n/2
!                             a(2*k+1) = I(k), 0<k<n/2
!                             a(1) = R(n/2)
!                     <case2>
!                         input data
!                             a(2*j) = R(j), 0<=j<n/2
!                             a(2*j+1) = I(j), 0<j<n/2
!                             a(1) = R(n/2)
!     [remark]
!         Inverse of 
!             call rdft(n, cos(pi/n), sin(pi/n), a)
!         is 
!             call rdft(n, cos(pi/n), -sin(pi/n), a)
!             do j = 0, n - 1
!                 a(j) = a(j) * 2 / n
!             end do
!         .
!
!
! -------- DCT (Discrete Cosine Transform) / Inverse of DCT --------
!     [definition]
!         <case1> IDCT (excluding scale)
!             C(k) = sum_j=0^n-1 a(j)*cos(pi*j*(k+1/2)/n), 0<=k<n
!         <case2> DCT
!             C(k) = sum_j=0^n-1 a(j)*cos(pi*(j+1/2)*k/n), 0<=k<n
!     [usage]
!         <case1>
!             call ddct(n, cos(pi/n/2), sin(pi/n/2), a)
!         <case2>
!             call ddct(n, cos(pi/n/2), -sin(pi/n/2), a)
!     [parameters]
!         n          :data length (integer)
!                     n >= 2, n = power of 2
!         a(0:n-1)   :input/output data (real*8)
!                     output data
!                         a(k) = C(k), 0<=k<n
!     [remark]
!         Inverse of 
!             call ddct(n, cos(pi/n/2), -sin(pi/n/2), a)
!         is 
!             a(0) = a(0) / 2
!             call ddct(n, cos(pi/n/2), sin(pi/n/2), a)
!             do j = 0, n - 1
!                 a(j) = a(j) * 2 / n
!             end do
!         .
!
!
! -------- DST (Discrete Sine Transform) / Inverse of DST --------
!     [definition]
!         <case1> IDST (excluding scale)
!             S(k) = sum_j=1^n A(j)*sin(pi*j*(k+1/2)/n), 0<=k<n
!         <case2> DST
!             S(k) = sum_j=0^n-1 a(j)*sin(pi*(j+1/2)*k/n), 0<k<=n
!     [usage]
!         <case1>
!             call ddst(n, cos(pi/n/2), sin(pi/n/2), a)
!         <case2>
!             call ddst(n, cos(pi/n/2), -sin(pi/n/2), a)
!     [parameters]
!         n          :data length (integer)
!                     n >= 2, n = power of 2
!         a(0:n-1)   :input/output data (real*8)
!                     <case1>
!                         input data
!                             a(j) = A(j), 0<j<n
!                             a(0) = A(n)
!                         output data
!                             a(k) = S(k), 0<=k<n
!                     <case2>
!                         output data
!                             a(k) = S(k), 0<k<n
!                             a(0) = S(n)
!     [remark]
!         Inverse of 
!             call ddst(n, cos(pi/n/2), -sin(pi/n/2), a)
!         is 
!             a(0) = a(0) / 2
!             call ddst(n, cos(pi/n/2), sin(pi/n/2), a)
!             do j = 0, n - 1
!                 a(j) = a(j) * 2 / n
!             end do
!         .
!
!
! -------- Cosine Transform of RDFT (Real Symmetric DFT) --------
!     [definition]
!         C(k) = sum_j=0^n a(j)*cos(pi*j*k/n), 0<=k<=n
!     [usage]
!         call dfct(n, cos(pi/n), sin(pi/n), a)
!     [parameters]
!         n          :data length - 1 (integer)
!                     n >= 2, n = power of 2
!         a(0:n)     :input/output data (real*8)
!                     output data
!                         a(k) = C(k), 0<=k<=n
!     [remark]
!         Inverse of 
!             a(0) = a(0) / 2
!             a(n) = a(n) / 2
!             call dfct(n, cos(pi/n), sin(pi/n), a)
!         is 
!             a(0) = a(0) / 2
!             a(n) = a(n) / 2
!             call dfct(n, cos(pi/n), sin(pi/n), a)
!             do j = 0, n
!                 a(j) = a(j) * 2 / n
!             end do
!         .
!
!
! -------- Sine Transform of RDFT (Real Anti-symmetric DFT) --------
!     [definition]
!         S(k) = sum_j=1^n-1 a(j)*sin(pi*j*k/n), 0<k<n
!     [usage]
!         call dfst(n, cos(pi/n), sin(pi/n), a)
!     [parameters]
!         n          :data length + 1 (integer)
!                     n >= 2, n = power of 2
!         a(0:n-1)   :input/output data (real*8)
!                     output data
!                         a(k) = S(k), 0<k<n
!                     (a(0) is used for work area)
!     [remark]
!         Inverse of 
!             call dfst(n, cos(pi/n), sin(pi/n), a)
!         is 
!             call dfst(n, cos(pi/n), sin(pi/n), a)
!             do j = 1, n - 1
!                 a(j) = a(j) * 2 / n
!             end do
!         .
!
!
      subroutine bitrv2(n, a)
      integer n, j, j1, k, k1, l, m, m2, n2
      real*8 a(0 : n - 1), xr, xi
      m = n / 4
      m2 = 2 * m
      n2 = n - 2
      k = 0
      do j = 0, m2 - 4, 4
          if (j .lt. k) then
              xr = a(j)
              xi = a(j + 1)
              a(j) = a(k)
              a(j + 1) = a(k + 1)
              a(k) = xr
              a(k + 1) = xi
          else if (j .gt. k) then
              j1 = n2 - j
              k1 = n2 - k
              xr = a(j1)
              xi = a(j1 + 1)
              a(j1) = a(k1)
              a(j1 + 1) = a(k1 + 1)
              a(k1) = xr
              a(k1 + 1) = xi
          end if
          k1 = m2 + k
          xr = a(j + 2)
          xi = a(j + 3)
          a(j + 2) = a(k1)
          a(j + 3) = a(k1 + 1)
          a(k1) = xr
          a(k1 + 1) = xi
          l = m
          do while (k .ge. l)
              k = k - l
              l = l / 2
          end do
          k = k + l
      end do
      end
!
      subroutine cdft(n, wr, wi, a)
      integer n, i, j, k, l, m
      real*8 wr, wi, a(0 : n - 1), wmr, wmi, wkr, wki, wdr, wdi, 
     &    ss, xr, xi
      wmr = wr
      wmi = wi
      m = n
      do while (m .gt. 4)
          l = m / 2
          wkr = 1
          wki = 0
          wdr = 1 - 2 * wmi * wmi
          wdi = 2 * wmi * wmr
          ss = 2 * wdi
          wmr = wdr
          wmi = wdi
          do j = 0, n - m, m
              i = j + l
              xr = a(j) - a(i)
              xi = a(j + 1) - a(i + 1)
              a(j) = a(j) + a(i)
              a(j + 1) = a(j + 1) + a(i + 1)
              a(i) = xr
              a(i + 1) = xi
              xr = a(j + 2) - a(i + 2)
              xi = a(j + 3) - a(i + 3)
              a(j + 2) = a(j + 2) + a(i + 2)
              a(j + 3) = a(j + 3) + a(i + 3)
              a(i + 2) = wdr * xr - wdi * xi
              a(i + 3) = wdr * xi + wdi * xr
          end do
          do k = 4, l - 4, 4
              wkr = wkr - ss * wdi
              wki = wki + ss * wdr
              wdr = wdr - ss * wki
              wdi = wdi + ss * wkr
              do j = k, n - m + k, m
                  i = j + l
                  xr = a(j) - a(i)
                  xi = a(j + 1) - a(i + 1)
                  a(j) = a(j) + a(i)
                  a(j + 1) = a(j + 1) + a(i + 1)
                  a(i) = wkr * xr - wki * xi
                  a(i + 1) = wkr * xi + wki * xr
                  xr = a(j + 2) - a(i + 2)
                  xi = a(j + 3) - a(i + 3)
                  a(j + 2) = a(j + 2) + a(i + 2)
                  a(j + 3) = a(j + 3) + a(i + 3)
                  a(i + 2) = wdr * xr - wdi * xi
                  a(i + 3) = wdr * xi + wdi * xr
              end do
          end do
          m = l
      end do
      if (m .gt. 2) then
          do j = 0, n - 4, 4
              xr = a(j) - a(j + 2)
              xi = a(j + 1) - a(j + 3)
              a(j) = a(j) + a(j + 2)
              a(j + 1) = a(j + 1) + a(j + 3)
              a(j + 2) = xr
              a(j + 3) = xi
          end do
      end if
      if (n .gt. 4) call bitrv2(n, a)
      end
!
      subroutine rdft(n, wr, wi, a)
      integer n, j, k
      real*8 wr, wi, a(0 : n - 1), wmr, wmi, wkr, wki, wdr, wdi, 
     &    ss, xr, xi, yr, yi
      if (n .gt. 4) then
          wkr = 0
          wki = 0
          wdr = wi * wi
          wdi = wi * wr
          ss = 4 * wdi
          wmr = 1 - 2 * wdr
          wmi = 2 * wdi
          if (wmi .ge. 0) then
              call cdft(n, wmr, wmi, a)
              xi = a(0) - a(1)
              a(0) = a(0) + a(1)
              a(1) = xi
          end if
          do k = n / 2 - 4, 4, -4
              j = n - k
              xr = a(k + 2) - a(j - 2)
              xi = a(k + 3) + a(j - 1)
              yr = wdr * xr - wdi * xi
              yi = wdr * xi + wdi * xr
              a(k + 2) = a(k + 2) - yr
              a(k + 3) = a(k + 3) - yi
              a(j - 2) = a(j - 2) + yr
              a(j - 1) = a(j - 1) - yi
              wkr = wkr + ss * wdi
              wki = wki + ss * (0.5d0 - wdr)
              xr = a(k) - a(j)
              xi = a(k + 1) + a(j + 1)
              yr = wkr * xr - wki * xi
              yi = wkr * xi + wki * xr
              a(k) = a(k) - yr
              a(k + 1) = a(k + 1) - yi
              a(j) = a(j) + yr
              a(j + 1) = a(j + 1) - yi
              wdr = wdr + ss * wki
              wdi = wdi + ss * (0.5d0 - wkr)
          end do
          j = n - 2
          xr = a(2) - a(j)
          xi = a(3) + a(j + 1)
          yr = wdr * xr - wdi * xi
          yi = wdr * xi + wdi * xr
          a(2) = a(2) - yr
          a(3) = a(3) - yi
          a(j) = a(j) + yr
          a(j + 1) = a(j + 1) - yi
          if (wmi .lt. 0) then
              a(1) = 0.5d0 * (a(0) - a(1))
              a(0) = a(0) - a(1)
              call cdft(n, wmr, wmi, a)
          end if
      else
          if (wi .lt. 0) then
              a(1) = 0.5d0 * (a(0) - a(1))
              a(0) = a(0) - a(1)
          end if
          if (n .gt. 2) then
              xr = a(0) - a(2)
              xi = a(1) - a(3)
              a(0) = a(0) + a(2)
              a(1) = a(1) + a(3)
              a(2) = xr
              a(3) = xi
          end if
          if (wi .ge. 0) then
              xi = a(0) - a(1)
              a(0) = a(0) + a(1)
              a(1) = xi
          end if
      end if
      end
!
      subroutine ddct(n, wr, wi, a)
      integer n, j, k, m
      real*8 wr, wi, a(0 : n - 1), wkr, wki, wdr, wdi, ss, xr
      if (n .gt. 2) then
          wkr = 0.5d0
          wki = 0.5d0
          wdr = 0.5d0 * (wr - wi)
          wdi = 0.5d0 * (wr + wi)
          ss = 2 * wi
          if (wi .lt. 0) then
              xr = a(n - 1)
              do k = n - 2, 2, -2
                  a(k + 1) = a(k) - a(k - 1)
                  a(k) = a(k) + a(k - 1)
              end do
              a(1) = 2 * xr
              a(0) = 2 * a(0)
              call rdft(n, 1 - ss * wi, ss * wr, a)
              xr = wdr
              wdr = wdi
              wdi = xr
              ss = -ss
          end if
          m = n / 2
          do k = 1, m - 3, 2
              j = n - k
              xr = wdi * a(k) - wdr * a(j)
              a(k) = wdr * a(k) + wdi * a(j)
              a(j) = xr
              wkr = wkr - ss * wdi
              wki = wki + ss * wdr
              xr = wki * a(k + 1) - wkr * a(j - 1)
              a(k + 1) = wkr * a(k + 1) + wki * a(j - 1)
              a(j - 1) = xr
              wdr = wdr - ss * wki
              wdi = wdi + ss * wkr
          end do
          k = m - 1
          j = n - k
          xr = wdi * a(k) - wdr * a(j)
          a(k) = wdr * a(k) + wdi * a(j)
          a(j) = xr
          a(m) = (wki + ss * wdr) * a(m)
          if (wi .ge. 0) then
              call rdft(n, 1 - ss * wi, ss * wr, a)
              xr = a(1)
              do k = 2, n - 2, 2
                  a(k - 1) = a(k) - a(k + 1)
                  a(k) = a(k) + a(k + 1)
              end do
              a(n - 1) = xr
          end if
      else
          if (wi .ge. 0) then
              xr = 0.5d0 * (wr + wi) * a(1)
              a(1) = a(0) - xr
              a(0) = a(0) + xr
          else
              xr = a(0) - a(1)
              a(0) = a(0) + a(1)
              a(1) = 0.5d0 * (wr - wi) * xr
          end if
      end if
      end
!
      subroutine ddst(n, wr, wi, a)
      integer n, j, k, m
      real*8 wr, wi, a(0 : n - 1), wkr, wki, wdr, wdi, ss, xr
      if (n .gt. 2) then
          wkr = 0.5d0
          wki = 0.5d0
          wdr = 0.5d0 * (wr - wi)
          wdi = 0.5d0 * (wr + wi)
          ss = 2 * wi
          if (wi .lt. 0) then
              xr = a(n - 1)
              do k = n - 2, 2, -2
                  a(k + 1) = a(k) + a(k - 1)
                  a(k) = a(k) - a(k - 1)
              end do
              a(1) = -2 * xr
              a(0) = 2 * a(0)
              call rdft(n, 1 - ss * wi, ss * wr, a)
              xr = wdr
              wdr = -wdi
              wdi = xr
              wkr = -wkr
          end if
          m = n / 2
          do k = 1, m - 3, 2
              j = n - k
              xr = wdi * a(j) - wdr * a(k)
              a(k) = wdr * a(j) + wdi * a(k)
              a(j) = xr
              wkr = wkr - ss * wdi
              wki = wki + ss * wdr
              xr = wki * a(j - 1) - wkr * a(k + 1)
              a(k + 1) = wkr * a(j - 1) + wki * a(k + 1)
              a(j - 1) = xr
              wdr = wdr - ss * wki
              wdi = wdi + ss * wkr
          end do
          k = m - 1
          j = n - k
          xr = wdi * a(j) - wdr * a(k)
          a(k) = wdr * a(j) + wdi * a(k)
          a(j) = xr
          a(m) = (wki + ss * wdr) * a(m)
          if (wi .ge. 0) then
              call rdft(n, 1 - ss * wi, ss * wr, a)
              xr = a(1)
              do k = 2, n - 2, 2
                  a(k - 1) = a(k + 1) - a(k)
                  a(k) = a(k) + a(k + 1)
              end do
              a(n - 1) = -xr
          end if
      else
          if (wi .ge. 0) then
              xr = 0.5d0 * (wr + wi) * a(1)
              a(1) = xr - a(0)
              a(0) = a(0) + xr
          else
              xr = a(0) + a(1)
              a(0) = a(0) - a(1)
              a(1) = 0.5d0 * (wr - wi) * xr
          end if
      end if
      end
!
      subroutine bitrv(n, a)
      integer n, j, k, l, m, m2, n1
      real*8 a(0 : n - 1), xr
      if (n .gt. 2) then
          m = n / 4
          m2 = 2 * m
          n1 = n - 1
          k = 0
          do j = 0, m2 - 2, 2
              if (j .lt. k) then
                  xr = a(j)
                  a(j) = a(k)
                  a(k) = xr
              else if (j .gt. k) then
                  xr = a(n1 - j)
                  a(n1 - j) = a(n1 - k)
                  a(n1 - k) = xr
              end if
              xr = a(j + 1)
              a(j + 1) = a(m2 + k)
              a(m2 + k) = xr
              l = m
              do while (k .ge. l)
                  k = k - l
                  l = l / 2
              end do
              k = k + l
          end do
      end if
      end
!
      subroutine dfct(n, wr, wi, a)
      integer n, j, k, m, mh
      real*8 wr, wi, a(0 : n), wmr, wmi, xr, xi, an
      wmr = wr
      wmi = wi
      m = n / 2
      do j = 0, m - 1
          k = n - j
          xr = a(j) + a(k)
          a(j) = a(j) - a(k)
          a(k) = xr
      end do
      an = a(n)
      do while (m .ge. 2)
          call ddct(m, wmr, wmi, a)
          xr = 1 - 2 * wmi * wmi
          wmi = 2 * wmi * wmr
          wmr = xr
          call bitrv(m, a)
          mh = m / 2
          xi = a(m)
          a(m) = a(0)
          a(0) = an - xi
          an = an + xi
          do j = 1, mh - 1
              k = m - j
              xr = a(m + k)
              xi = a(m + j)
              a(m + j) = a(j)
              a(m + k) = a(k)
              a(j) = xr - xi
              a(k) = xr + xi
          end do
          xr = a(mh)
          a(mh) = a(m + mh)
          a(m + mh) = xr
          m = mh
      end do
      xi = a(1)
      a(1) = a(0)
      a(0) = an + xi
      a(n) = an - xi
      call bitrv(n, a)
      end
!
      subroutine dfst(n, wr, wi, a)
      integer n, j, k, m, mh
      real*8 wr, wi, a(0 : n - 1), wmr, wmi, xr, xi
      wmr = wr
      wmi = wi
      m = n / 2
      do j = 1, m - 1
          k = n - j
          xr = a(j) - a(k)
          a(j) = a(j) + a(k)
          a(k) = xr
      end do
      a(0) = a(m)
      do while (m .ge. 2)
          call ddst(m, wmr, wmi, a)
          xr = 1 - 2 * wmi * wmi
          wmi = 2 * wmi * wmr
          wmr = xr
          call bitrv(m, a)
          mh = m / 2
          do j = 1, mh - 1
              k = m - j
              xr = a(m + k)
              xi = a(m + j)
              a(m + j) = a(j)
              a(m + k) = a(k)
              a(j) = xr + xi
              a(k) = xr - xi
          end do
          a(m) = a(0)
          a(0) = a(m + mh)
          a(m + mh) = a(mh)
          m = mh
      end do
      a(1) = a(0)
      a(0) = 0
      call bitrv(n, a)
      end
!
