#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "boo2/BooObject.hpp"
#include "boo2/audiodev/IAudioVoiceEngine.hpp"
#include "AudioSubmix.hpp"
#include "AudioVoice.hpp"
#include "Common.hpp"
#include "LtRtProcessing.hpp"

namespace boo2 {

/** Base class for managing mixing and sample-rate-conversion amongst active voices */
class BaseAudioVoiceEngine : public IAudioVoiceEngine {
protected:
  friend class AudioVoice;
  friend class AudioSubmix;
  friend class AudioVoiceMono;
  friend class AudioVoiceStereo;
  float m_totalVol = 1.f;
  AudioVoiceEngineMixInfo m_mixInfo;
  std::recursive_mutex m_dataMutex;
  AudioVoice* m_voiceHead = nullptr;
  AudioSubmix* m_submixHead = nullptr;
  size_t m_5msFrames = 0;
  IAudioVoiceEngineCallback* m_engineCallback = nullptr;

  /* Shared scratch buffers for accumulating audio data for resampling */
  std::vector<int16_t> m_scratchIn;
  std::vector<float> m_scratchPre;
  std::vector<float> m_scratchPost;

  /* LtRt processing if enabled */
  std::unique_ptr<LtRtProcessing> m_ltRtProcessing;
  std::vector<float> m_ltRtIn;

  std::unique_ptr<AudioSubmix> m_mainSubmix;
  std::list<AudioSubmix*> m_linearizedSubmixes;
  bool m_submixesDirty = true;

  void _pumpAndMixVoices(size_t frames, float* dataOut);

  void _resetSampleRate();

public:
  BaseAudioVoiceEngine() : m_mainSubmix(std::make_unique<AudioSubmix>(*this, nullptr, -1, false)) {}
  ~BaseAudioVoiceEngine() override;
  ObjToken<IAudioVoice> allocateNewMonoVoice(double sampleRate, IAudioVoiceCallback* cb,
                                             bool dynamicPitch = false) override;

  ObjToken<IAudioVoice> allocateNewStereoVoice(double sampleRate, IAudioVoiceCallback* cb,
                                               bool dynamicPitch = false) override;

  ObjToken<IAudioSubmix> allocateNewSubmix(bool mainOut, IAudioSubmixCallback* cb, int busId) override;

  void setCallbackInterface(IAudioVoiceEngineCallback* cb) override;

  void setVolume(float vol) override;
  bool enableLtRt(bool enable) override;
  const AudioVoiceEngineMixInfo& mixInfo() const;
  const AudioVoiceEngineMixInfo& clientMixInfo() const;
  AudioChannelSet getAvailableSet() override { return clientMixInfo().m_channels; }
  void pumpAndMixVoices() override {}
  size_t get5MsFrames() const override { return m_5msFrames; }
};

} // namespace boo2