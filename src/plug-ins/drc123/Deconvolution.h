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
#include "DataVector.h"
#include "Generate.h"

#include <fftw3.h>
#include <cpp/smartptr.h>
#include <cpp/event.h>


class Deconvolution : public Filter
{public: // Configuration interface
  enum WFN
  { WFN_NONE,
    WFN_DIMMEDHAMMING,
    WFN_HAMMING
  };
  struct Parameters
  { bool        Enabled;        ///< Flag whether the Filter is enabled by the user.
    int         FIROrder;       ///< Filter kernel length
    int         PlanSize;       ///< Last FFT plan size used for setup
    xstring     TargetFile;
    WFN         WindowFunction; ///< Currently selected window function
  };

  struct KernelChangeEventArgs
  { const Parameters* Params;
    unsigned    Channel;
    unsigned    Samplerate;
    const FreqDomainData* FreqDomain;
    double      FreqScale;
    const TimeDomainData* TimeDomain;
    double      TimeScale;
  };
  struct InstanceEventArgs
  { Deconvolution& Sender;
    bool        Destruction;
    InstanceEventArgs(Deconvolution& sender, bool destruction) : Sender(sender), Destruction(destruction) {}
  };

 private:
  class ParameterSet : public Iref_count, public Parameters
  {private:
    ParameterSet();
   public:
    static ParameterSet Default;
    Generate::TargetFile Target;
    ParameterSet(const Parameters& r);
  };
  /// Currently configured parameter set
  static volatile int_ptr<ParameterSet> ParamSet;

 private: // Working set
  int_ptr<ParameterSet> LocalParamSet;
  bool          NeedInit;
  bool          NeedFIR;
  bool          NeedKernel;
  bool          LastEnabled;///< Flag whether the Filter was enabled at the last call to \c InRequestBuffer
  int           CurPlanSize;///< Plan size for the FFT convolution
  int           CurPlanSize21;///< Plan size / 2 + 1 = count of complex FFT numbers
  int           CurFIROrder;///< Filter kernel length
  int           CurFIROrder2;///< Filter kernel length / 2

  FORMAT_INFO2  Format;

  PM123_TIME    PosMarker;  ///< Starting point of the Inbox buffer
  int           InboxLevel; ///< Number of samples in Inbox buffer
  int           Latency;    ///< Samples to discard before passing them to the output stage
  bool          Discard;    ///< true: discard buffer content before next request_buffer
  PM123_TIME    TempPos;    ///< Position returned during discard
 private: // FFT working set
  float*        Inbox;      ///< Buffer to collect incoming samples
  TimeDomainData TimeDomain;///< Buffer in the time domain
  FreqDomainData FreqDomain;///< Buffer in the frequency domain (shared memory with design)
  FreqDomainData Kernel[2]; ///< Frequency domain representation of the filter kernel.
  float*        Overlap[2]; ///< Keep old samples for convolution
  fftwf_complex* ChannelSave;///< Buffer to keep the frequency domain of a mono input for a second convolution
  fftwf_plan    ForwardPlan;///< FFTW plan for TimeDomain -> FreqDomain
  fftwf_plan    BackwardPlan;///< FFTW plan for FreqDomain -> TimeDomain

  /// Event, when the filter kernel changes.
  static event<const KernelChangeEventArgs> KernelChange;
  static volatile unsigned KernelUpdateRequest;

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
  void          Convolve(fftwf_complex* sp, fftwf_complex* kp);
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
  virtual ULONG InCommand(ULONG msg, const OUTPUT_PARAMS2* info);
  virtual int   InRequestBuffer(const FORMAT_INFO2* format, float** buf);
  virtual void  InCommitBuffer(int len, PM123_TIME pos);
 public:
  Deconvolution(FILTER_PARAMS2& params);
  virtual ~Deconvolution();

  static void   SetParameters(const Parameters& params) { ParamSet = new ParameterSet(params); }
  static void   GetParameters(Parameters& params)       { params = *int_ptr<ParameterSet>(ParamSet); }
  static void   GetDefaultParameters(Parameters& params){ params = ParameterSet::Default; }

  static event_pub<const KernelChangeEventArgs>& GetKernelChange() { return KernelChange; }
  static void   ForceKernelChange()                     { KernelUpdateRequest = true; }
};

#endif // DECONVOLUTION_H
