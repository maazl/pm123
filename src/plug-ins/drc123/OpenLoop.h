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

#ifndef OPENLOOP_H
#define OPENLOOP_H

#include "Filter.h"
#include "DataFile.h"
#include "DataVector.h"

#include <cpp/xstring.h>
#include <cpp/mutex.h>
#include <cpp/event.h>
#include <cpp/smartptr.h>
#include <fftw3.h>
#include <math.h>


/**
 * Abstract base class for open loop measurements.
 */
class OpenLoop : public Filter
{public:
  /// Type of the reference signal.
  enum Mode
  { /// The reference is single channel/mono.
    RFM_MONO    = 0x00,
    /// Left channel only
    RFM_LEFT    = 0x01,
    /// Right channel only
    RFM_RIGHT   = 0x02,
    /// @brief The reference has two orthogonal signals for both channels with distinct frequencies.
    /// @details This mode causes all every second frequency to appear at one channel while the other frequencies are on the other channel.
    RFM_STEREO  = 0x03,
    /// The reference is single channel, but the output on the right channel is inverted.
    /// @remarks This could be used with appropriate hardware to suppress common mode noise.
    RFM_DIFFERENTIAL = 0x04
  };
  /// Configuration parameters for OpenLoop. You may extend them.
  struct Parameters
  { /// FFT size for reference and analysis.
    unsigned    FFTSize;
    /// Discard number of samples when recording starts.
    unsigned    DiscardSamp;
    /// Minimum frequency to analyze.
    double      RefFMin;
    /// Maximum frequency to analyze.
    double      RefFMax;
    /// @brief Energy distribution of the reference signal.
    /// @details The Energy in the reference follows the rule P ~ f^RefExponent.
    double      RefExponent;
    /// Do not spend energy in even frequencies.
    bool        RefSkipEven;
    /// Skip some random frequencies in stereo mode to break symmetry.
    bool        RefSkipRand;
    /// Type of the reference signal.
    Mode        RefMode;
    /// Reference playback volume
    double      RefVolume;
    /// Minimum increment factor between neighbor frequencies in the reference signal.
    double      RefFreqFactor;
    /// Distribute energy of unused frequencies to their neighbors.
    bool        RefEnergyDist;
    /// Number of harmonics for the line frequency notch. 0 = off, 1 = line Frequency only.
    unsigned    LineNotchHarmonics;
    /// Notch filter for line frequency.
    double      LineNotchFreq;
    /// Increment factor to aggregate adjacent frequencies in the interval [f..f+f*FBin].
    double      AnaFBin;
    /// Swap channels on input processing.
    bool        AnaSwap;

    // GUI injection
    double      GainLow, GainHigh;    ///< Display range for gain response
    double      DelayLow, DelayHigh;  ///< Display range for delay
    double      VULow;
    double      VUYellow, VURed;
  };
  class OpenLoopFile
  : public DataFile
  , public Parameters
  {public:
    /// Average group delay
    double      AverageDelay;
    /// Number of phase unwraps.
    unsigned    PhaseUnwrapCount;
    /// Number of times the phase unwrapping was ambiguous.
    unsigned    IndeterminatePhase;
   protected:
    virtual bool ParseHeaderField(const char* string);
    virtual bool WriteHeaderFields(FILE* f);
   public:
                OpenLoopFile(unsigned cols) : DataFile(cols), AverageDelay(0.), PhaseUnwrapCount(0), IndeterminatePhase(0) {}
    virtual bool Load(const char* filename = NULL, bool nodata = false);
  };
  /// Info about input for VU meter
  struct Statistics
  { double      Sum2;
    double      Peak;
    unsigned long Count;
    double      RMSdB() const   { return log(Sum2/(double)Count) * (20. / M_LN10 / 2.); }
    double      PeakdB() const  { return log(Peak) * (20. / M_LN10); }
    void        Add(const float* data, unsigned inc, unsigned count);
    void        Add(const Statistics& r);
    void        Reset()         { Sum2 = 0; Count = 0; Peak = 0; }
    Statistics()                { Reset(); }
  };
  /// Virtual table for static Functions provided by derived classes.
  /// @remarks A singleton with virtual function would do the Job as well,
  /// but that would require two related classes. One for the singleton
  /// and another one for active filter plug-in instances.
  struct SVTable
  { bool (*IsRunning)();
    bool (*Start)();
    bool (*Stop)();
    void (*Clear)();
    void (*SetVolume)(double volume);
    //SyncAccess<OpenLoopFile>* (*AccessData)();
    event_pub<const int>& EvDataUpdate;
  };
 protected:
  struct SampleData
  : public Iref_count
  , public TimeDomainData
  { unsigned LoopCount;
    SampleData(size_t len) : TimeDomainData(len) {}
  };

 public:
  /// Recording URI for measurements.
  static volatile xstring RecURI;
 protected:
  static event<const int> EvDataUpdate;
 protected:
  Parameters    Params;
  void          DLLENTRYP(OutEvent)(Filter* w, OUTEVENTTYPE event);
  Filter*       OutW;
  /// Sampling frequency of the input and output data and number of channels in the \e input data.
  /// @remarks The number of channels in the \e output data is in VFormat.channels.
  FORMAT_INFO2  Format;
 private: // ref generator state
  /// Thread ID of reference generator
  int           RefTID;
  /// true if the output device sent \c OUTEVENT_LOW_WATER,
  /// false if the output device sent \c OUTEVENT_HIGH_WATER.
  bool          IsLowWater;
  /// The reference function has been generated.
  volatile bool InitComplete;
  /// Request to terminate the generator thread.
  volatile bool Terminate;
  /// @brief Design function of reference signal of channel 1/2.
  /// @details Channel 2 is only valid if RefMode == RFM_STEREO.
  FreqDomainData RefDesign[2];
  /// FFT length which is unique with respect to the auto correlation of the reference.
  unsigned      UncorrelatedFFTSize;
 protected:
  /// Raw data of reference signal.
  TimeDomainData RefData;
  /// Temporary storage for FFT data.
  FreqDomainData AnaTemp;
 private: // analysis state
  int           AnaTID;
  Event         AnaEvent;
  size_t        ResultLevel;
  volatile int_ptr<SampleData> CurrentData;
  int_ptr<SampleData> NextData;
  unsigned      LoopCount;
  FreqDomainData AnaFFT[2];
  TimeDomainData ResCross;
  fftwf_plan    AnaPlan;
  fftwf_plan    CrossPlan;
  /// Request to clear the aggregate buffer.
  static volatile bool ClearRq;
 private: // internal storage of filter plug-in
  OUTPUT_PARAMS2 VParams;
  INFO_BUNDLE_CV VInfo;
  TECH_INFO     VTechInfo;
 private: // Statistics
  static Statistics Stat[2];
  static Mutex  StatMtx;

 private:
  PROXYFUNCDEF void DLLENTRY InEventProxy(Filter* w, OUTEVENTTYPE event);
 private: // Reference generator
  PROXYFUNCDEF void TFNENTRY RefThreadStub(void* arg);
          void  RefThreadFunc();
          void  OverrideTechInfo(const OUTPUT_PARAMS2*& target);
 protected:
  /// @brief Generate the reference signal.
  /// @details This function is called once when the playback starts.
  /// @post The function must ensure that RefData contains valid data.
  /// @remarks In general the default implementation should be sufficient.
  /// But you can override this to create more esoteric reference signals.
  virtual void  GenerateRef();
 private: // Analysis thread
  PROXYFUNCDEF void TFNENTRY AnaThreadStub(void* arg);
          void  AnaThreadFunc();
 protected:
  /// You may override this function to do some initialization stuff
  /// in the analyzer thread. It is called before any data is processed by ProcessFFTData.
  virtual void  InitAnalyzer();
  /// Calculate the cross correlation in a temporary buffer.
  /// @param signal Signal to correlate.
  /// @param reference to correlate with.
  /// @return Result in a temporary buffer. This is valid until the next call to \c CrossCorrelate
  /// or this instance dies. Note that the result is not normalized.
  /// @pre @code signal.size() == ref.size() == Params.FFTSize/2+1 @endcode
  TimeDomainData& CrossCorrelate(const FreqDomainData& signal, const FreqDomainData& ref);
  /// Estimate the most likely delay by cross correlation.
  /// @param signal FFT of the signal to estimate.
  /// @param ref FFT of the reference. This also controls the weighting.
  /// @param response [out] reliability and sign of the match.
  /// The value returned contains the relative amount of energy in the match and the sign of the match.
  /// @return Most likely delay of \a fd1 against \a fd2 in samples.
  /// @post The returned value will always be in the interval [0,T]
  /// with T = Params.FFTSize / Format.samplerate.
          int   ComputeDelay(const FreqDomainData& signal, const FreqDomainData& ref, double& response);
  /// @brief Calculate the average delay with phase unwrapping.
  /// @details The function takes care of ambiguities in the autocorrelation
  /// of the reference signal, i.e. \c UncorrelatedFFTSize.
  /// @param delay1 1st delay in samples.
  /// @param delay2 2nd delay in samples.
  /// @return average delay in samples.
          int   AverageDelay(int delay1, int delay2);
  /// @brief Process a block of input data.
  /// @details The function is called once for every \c FFTSize number of input samples.
  /// You can override it by the appropriate analysis function.
  /// @param input Array with the input samples in native PM123 format (interleaved).
  /// There are always \c FFTSize samples in the array.
  /// You must not change the array size, but you can take a strong reference with \c int_ptr<SampleData>.
  /// @remarks The default implementation calculate the FFT of both channels in the input data
  /// and passes the result to \c ProcessFFTData.
  virtual void  ProcessInput(SampleData& input);
  /// @brief Process a block of input data after FFT transformation.
  /// @param input Array of two channels in frequency domain representation.
  /// @param scale All values in input need to be multiplied by this scale value.
  virtual void  ProcessFFTData(FreqDomainData (&input)[2], double scale);

 protected:
  virtual ULONG InCommand(ULONG msg, const OUTPUT_PARAMS2* info);
  virtual int   InRequestBuffer(const FORMAT_INFO2* format, float** buf);
  virtual void  InCommitBuffer(int len, PM123_TIME pos);
  virtual void  InEvent(OUTEVENTTYPE event);
 public:
                OpenLoop(const Parameters& params, FILTER_PARAMS2& filterparams);
  virtual       ~OpenLoop();
 protected:
  static  void  SetVolume(double volume);
  static  bool  Start(FilterMode mode, double volume);
          void  TerminateRequest()  { Terminate = true; AnaEvent.Set(); }
          void  Fail(const char* message);
 public:
  static  bool  Stop();
  static  void  Clear() { ClearRq = true; }
  const FreqDomainData& GetRefDesign(unsigned channel) { return RefDesign[channel && (Params.RefMode & RFM_STEREO)]; }
  static  void  GetStatistics(Statistics (&stat)[2]);
};

#endif // OPENLOOP_H
