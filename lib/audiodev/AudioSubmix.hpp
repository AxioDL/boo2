#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <list>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "boo2/audiodev/IAudioSubmix.hpp"
#include "../Common.hpp"

#if __SSE__
#include <immintrin.h>
#endif

struct AudioUnitVoiceEngine;
struct VSTVoiceEngine;
struct WAVOutVoiceEngine;

namespace boo2 {
class BaseAudioVoiceEngine;
class AudioVoice;
struct AudioVoiceEngineMixInfo;
/* Output gains for each mix-send/channel */

class AudioSubmix : public ListNode<AudioSubmix, BaseAudioVoiceEngine*, IAudioSubmix> {
  friend class BaseAudioVoiceEngine;
  friend class AudioVoiceMono;
  friend class AudioVoiceStereo;
  friend struct WASAPIAudioVoiceEngine;
  friend struct ::AudioUnitVoiceEngine;
  friend struct ::VSTVoiceEngine;
  friend struct ::WAVOutVoiceEngine;

  /* Mixer-engine relationships */
  int m_busId;
  bool m_mainOut;

  /* Callback (effect source, optional) */
  IAudioSubmixCallback* m_cb;

  /* Slew state for output gains */
  size_t m_slewFrames = 0;
  size_t m_curSlewFrame = 0;

  /* Output gains for each mix-send/channel */
  std::unordered_map<IAudioSubmix*, std::array<float, 2>> m_sendGains;

  /* Temporary scratch buffers for accumulating submix audio */
  std::vector<float> m_scratch;

  /* Override scratch buffers with alternate destination */
  float* m_redirect = nullptr;

  /* C3-linearization support (to mitigate a potential diamond problem on 'clever' submix routes) */
  bool _isDirectDependencyOf(AudioSubmix* send);
  std::list<AudioSubmix*> _linearizeC3();
  static bool _mergeC3(std::list<AudioSubmix*>& output, std::vector<std::list<AudioSubmix*>>& lists);

  /* Fill scratch buffers with silence for new mix cycle */
  void _zeroFill();

  /* Receive audio from a single voice / submix */
  float* _getMergeBuf(size_t frames);

  /* Mix scratch buffers into sends */
  size_t _pumpAndMix(size_t frames);

  void _resetOutputSampleRate();

public:
  static AudioSubmix*& _getHeadPtr(BaseAudioVoiceEngine* head);
  static std::unique_lock<std::recursive_mutex> _getHeadLock(BaseAudioVoiceEngine* head);

  AudioSubmix(BaseAudioVoiceEngine& root, IAudioSubmixCallback* cb, int busId, bool mainOut);
  ~AudioSubmix() override;

  void resetSendLevels() override;
  void setSendLevel(IAudioSubmix* submix, float level, bool slew) override;
  const AudioVoiceEngineMixInfo& mixInfo() const;
  double getSampleRate() const override;
};

} // namespace boo2