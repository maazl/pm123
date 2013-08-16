/*
 * Copyright 2013-2013 Marcel Mueller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DECONVOLUTION_H
#define DECONVOLUTION_H

#include "Filter.h"

#include <fftw3.h>
#include <cpp/smartptr.h>


/** Target response coefficient */
struct Coeff
{
  /// natural logarithm of frequency
  float       LF;
  /// natural logarithm of gain value
  float       LV;
  /// group delay [s]
  float       GD;
};

/** Target frequency response */
struct TargetResponse : public Iref_count
{
  /// @brief This array contains the target response of the filter ordered by frequency.
  /// @details The first entry must be below any frequency that is ever requested
  /// and the last entry must be above any frequency that is ever requested.
  /// Otherwise the behavior of the interpolation is undefined.
  sco_arr<Coeff> Channels[2];
};

class Deconvolution : public Filter
{public: // Configuration interface
  enum WFN
  { WFN_NONE,
    WFN_DIMMEDHAMMING,
    WFN_HAMMING
  };
  struct Parameters
  { bool        Enabled;
    int         FIROrder;
    int         PlanSize;
    xstring     FilterFile;
    WFN         WindowFunction; ///< Currently selected window function
    // computed
    bool        IsValid();
  };

 private:
  struct ParameterSet
  : public Iref_count
  , public Parameters
  {protected:
    ParameterSet() {}
   public:
    ParameterSet(const Parameters& r);
    sco_arr<Coeff> TargetResponse[2];
  };
  struct DefaultParameters : public ParameterSet
  { DefaultParameters();
  };
  /// Currently configured parameter set
  static volatile int_ptr<ParameterSet> ParamSet;

 private: // Working set
  bool          NeedInit;
  bool          NeedFIR;
  bool          NeedKernel;
  bool          Enabled;    ///< Flag whether the EQ was enabled at the last call to InRequestBuffer
  int           CurFIROrder;///< Filter kernel length
  int           CurFIROrder2;///< Filter kernel length / 2
  int           CurPlanSize;///< Plansize for the FFT convolution

  FORMAT_INFO2  Format;

  PM123_TIME    PosMarker;  ///< Starting point of the Inbox buffer
  int           InboxLevel; ///< Number of samples in Inbox buffer
  int           Latency;    ///< Samples to discard before passing them to the output stage
  bool          Discard;    ///< true: discard buffer content before next request_buffer
  PM123_TIME    TempPos;    ///< Position returned during discard
 private: // FFT working set
  float*        Inbox;      ///< Buffer to collect incoming samples
  float*        TimeDomain; ///< Buffer in the time domain
  fftwf_complex* FreqDomain;///< Buffer in the frequency domain (shared memory with design)
  fftwf_complex* Kernel[2]; ///< Since the kernel is real even, it's even real in the time domain
  float*        Overlap[2]; ///< Keep old samples for convolution
  fftwf_complex* ChannelSave;///< Buffer to keep the frequency domain of a mono input for a second convolution
  fftwf_plan    ForwardPlan;///< FFTW plan for TimeDomain -> FreqDomain
  fftwf_plan    BackwardPlan;///< FFTW plan for FreqDomain -> TimeDomain

 private:
  static double NoWindow(double);
  static double HammingWindow(double pos);
  static double DimmedHammingWindow(double pos);
  /// Fetch saved sampled
  void          LoadOverlap(float* overlap_buffer);
  /// Store samples
  void          SaveOverlap(float* overlap_buffer, int len);
  /// @brief Convert samples from short to float and store it in the \c TimeDomain buffer.
  /// @details The samples are shifted by \c FIROrder/2 to the right.
  /// \c FIROrder samples before the starting position are taken from \a overlap_buffer.
  /// The \a overlap_buffer is updated with the last \c FIROrder samples before return.
  void          LoadSamplesMono(const float* sp, const int len, float* overlap_buffer);
  /// @brief Convert samples from short to float and store it in the TimeDomain buffer.
  /// @details This will cover only one channel. The only difference to to mono-version is that
  /// the source pointer is incremented by 2 per sample.
  void          LoadSamplesStereo(const float* sp, const int len, float* overlap_buffer);
  /// @brief Store stereo samples.
  /// @details This will cover only one channel. The only difference to to mono-version is that
  /// the destination pointer is incremented by 2 per sample.
  void          StoreSamplesStereo(float* sp, const int len);
  /// Do convolution and back transformation.
  void          Convolute(fftwf_complex* sp, fftwf_complex* kp);
  void          FilterSamplesFFT(float* newsamples, const float* buf, int len);
  /// Only update the overlap buffer, no filtering.
  void          FilterSamplesNewOverlap(const float* buf, int len);
  /// Proxy functions to remove the first samples to compensate for the filter delay.
  int           DoRequestBuffer(float** buf);
  void          DoCommitBuffer(int len, PM123_TIME pos);
  /// @brief Applies the FFT filter to the current \c Inbox content and sends the result to the next plug-in.
  /// @pre You should not call this function with \c InboxLevel == 0.
  void          FilterAndSend();
  void          TrashBuffers();
  void          LocalFlush();
  /// Release FFT resources.
  void          FFTDestroy();
  /// Check for new parameters and adjust Need... and TargetResponse.
  /// Setup FIR kernel.
  bool          Setup();

 protected:
  virtual ULONG InCommand(ULONG msg, OUTPUT_PARAMS2* info);
  virtual int   InRequestBuffer(const FORMAT_INFO2* format, float** buf);
  virtual void  InCommitBuffer(int len, PM123_TIME pos);
 public:
  Deconvolution(FILTER_PARAMS2& params);
  virtual ~Deconvolution();

  static void   SetParameters(const Parameters& params) { ParamSet = new ParameterSet(params); }
  static void   GetParameters(Parameters& params) { params = *int_ptr<ParameterSet>(ParamSet); }
};

#endif // DECONVOLUTION_H
