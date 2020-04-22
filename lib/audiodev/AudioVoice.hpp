#pragma once

#include <mutex>
#include <unordered_map>

#include "boo2/audiodev/IAudioVoice.hpp"
#include "AudioMatrix.hpp"
#include "AudioVoiceEngine.hpp"
#include "Common.hpp"

#include <soxr.h>

struct AudioUnitVoiceEngine;
struct VSTVoiceEngine;
struct WAVOutVoiceEngine;

namespace boo2 {
class BaseAudioVoiceEngine;
struct AudioVoiceEngineMixInfo;
struct IAudioSubmix;

class AudioVoice : public ListNode<AudioVoice, BaseAudioVoiceEngine*, IAudioVoice> {
  friend class BaseAudioVoiceEngine;
  friend class AudioSubmix;
  friend struct WASAPIAudioVoiceEngine;
  friend struct ::AudioUnitVoiceEngine;
  friend struct ::VSTVoiceEngine;
  friend struct ::WAVOutVoiceEngine;

protected:
  /* Callback (audio source) */
  IAudioVoiceCallback* m_cb;

  /* Sample-rate converter */
  soxr_t m_src = nullptr;
  double m_sampleRateIn;
  double m_sampleRateOut;
  bool m_dynamicRate;

  /* Running bool */
  bool m_running = false;

  /* Deferred sample-rate reset */
  bool m_resetSampleRate = false;
  double m_deferredSampleRate;
  virtual void _resetSampleRate(double sampleRate) = 0;

  /* Deferred pitch ratio set */
  bool m_setPitchRatio = false;
  double m_pitchRatio = 1.0;
  double m_sampleRatio = 1.0;
  bool m_slew = false;
  void _setPitchRatio(double ratio, bool slew);

  /* Mid-pump update */
  void _midUpdate();

  virtual size_t pumpAndMix(size_t frames) = 0;

  AudioVoice(BaseAudioVoiceEngine& root, IAudioVoiceCallback* cb, bool dynamicRate);

public:
  static AudioVoice*& _getHeadPtr(BaseAudioVoiceEngine* head);
  static std::unique_lock<std::recursive_mutex> _getHeadLock(BaseAudioVoiceEngine* head);

  ~AudioVoice() override;
  void resetSampleRate(double sampleRate) override;
  void setPitchRatio(double ratio, bool slew) override;
  void start() override;
  void stop() override;
  double getSampleRateIn() const { return m_sampleRateIn; }
  double getSampleRateOut() const { return m_sampleRateOut; }
};

class AudioVoiceMono : public AudioVoice {
  std::unordered_map<IAudioSubmix*, AudioMatrixMono> m_sendMatrices;
  bool m_silentOut = false;
  void _resetSampleRate(double sampleRate) override;
  static size_t SRCCallback(AudioVoiceMono* ctx, int16_t** data, size_t requestedLen);
  bool isSilent() const;
  size_t pumpAndMix(size_t frames) override;

public:
  AudioVoiceMono(BaseAudioVoiceEngine& root, IAudioVoiceCallback* cb, double sampleRate, bool dynamicRate);
  void resetChannelLevels() override;
  void setMonoChannelLevels(IAudioSubmix* submix, const float coefs[8], bool slew) override;
  void setStereoChannelLevels(IAudioSubmix* submix, const float coefs[8][2], bool slew) override;
};

class AudioVoiceStereo : public AudioVoice {
  std::unordered_map<IAudioSubmix*, AudioMatrixStereo> m_sendMatrices;
  bool m_silentOut = false;
  void _resetSampleRate(double sampleRate) override;
  static size_t SRCCallback(AudioVoiceStereo* ctx, int16_t** data, size_t requestedLen);
  bool isSilent() const;
  size_t pumpAndMix(size_t frames) override;

public:
  AudioVoiceStereo(BaseAudioVoiceEngine& root, IAudioVoiceCallback* cb, double sampleRate, bool dynamicRate);
  void resetChannelLevels() override;
  void setMonoChannelLevels(IAudioSubmix* submix, const float coefs[8], bool slew) override;
  void setStereoChannelLevels(IAudioSubmix* submix, const float coefs[8][2], bool slew) override;
};

} // namespace boo2