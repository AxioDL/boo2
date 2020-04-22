#include "AudioVoiceEngine.hpp"

#include <cassert>
#include <cstring>

namespace boo2 {

BaseAudioVoiceEngine::~BaseAudioVoiceEngine() {
  m_mainSubmix.reset();
  assert(m_voiceHead == nullptr && "Dangling voices detected");
  assert(m_submixHead == nullptr && "Dangling submixes detected");
}

void BaseAudioVoiceEngine::_pumpAndMixVoices(size_t frames, float* dataOut) {
  if (dataOut)
    memset(dataOut, 0, sizeof(float) * frames * m_mixInfo.m_channelMap.m_channelCount);

  if (m_ltRtProcessing) {
    size_t sampleCount = m_5msFrames * 5;
    if (m_ltRtIn.size() < sampleCount)
      m_ltRtIn.resize(sampleCount);
    m_mainSubmix->m_redirect = m_ltRtIn.data();
  } else {
    m_mainSubmix->m_redirect = dataOut;
  }

  if (m_submixesDirty) {
    m_linearizedSubmixes = m_mainSubmix->_linearizeC3();
    m_submixesDirty = false;
  }

  size_t remFrames = frames;
  while (remFrames) {
    size_t thisFrames;
    if (remFrames < m_5msFrames) {
      thisFrames = remFrames;
      if (m_engineCallback)
        m_engineCallback->on5MsInterval(*this, thisFrames / double(m_5msFrames) * 5.0 / 1000.0);
    } else {
      thisFrames = m_5msFrames;
      if (m_engineCallback)
        m_engineCallback->on5MsInterval(*this, 5.0 / 1000.0);
    }

    if (m_ltRtProcessing)
      std::fill(m_ltRtIn.begin(), m_ltRtIn.end(), 0.f);

    for (auto it = m_linearizedSubmixes.rbegin(); it != m_linearizedSubmixes.rend(); ++it)
      (*it)->_zeroFill();

    if (m_voiceHead)
      for (AudioVoice& vox : *m_voiceHead)
        if (vox.m_running)
          vox.pumpAndMix(thisFrames);

    for (auto it = m_linearizedSubmixes.rbegin(); it != m_linearizedSubmixes.rend(); ++it)
      (*it)->_pumpAndMix(thisFrames);

    remFrames -= thisFrames;
    if (!dataOut)
      continue;

    if (m_ltRtProcessing) {
      m_ltRtProcessing->Process(m_ltRtIn.data(), dataOut, int(thisFrames));
      m_mainSubmix->m_redirect = m_ltRtIn.data();
    }

    size_t sampleCount = thisFrames * m_mixInfo.m_channelMap.m_channelCount;
    for (size_t i = 0; i < sampleCount; ++i)
      dataOut[i] *= m_totalVol;

    dataOut += sampleCount;
  }

  if (m_engineCallback)
    m_engineCallback->onPumpCycleComplete(*this);
}

void BaseAudioVoiceEngine::_resetSampleRate() {
  if (m_voiceHead)
    for (AudioVoice& vox : *m_voiceHead)
      vox._resetSampleRate(vox.m_sampleRateIn);
  if (m_submixHead)
    for (AudioSubmix& smx : *m_submixHead)
      smx._resetOutputSampleRate();
}

ObjToken<IAudioVoice> BaseAudioVoiceEngine::allocateNewMonoVoice(double sampleRate, IAudioVoiceCallback* cb,
                                                                 bool dynamicPitch) {
  return {new AudioVoiceMono(*this, cb, sampleRate, dynamicPitch)};
}

ObjToken<IAudioVoice> BaseAudioVoiceEngine::allocateNewStereoVoice(double sampleRate, IAudioVoiceCallback* cb,
                                                                   bool dynamicPitch) {
  return {new AudioVoiceStereo(*this, cb, sampleRate, dynamicPitch)};
}

ObjToken<IAudioSubmix> BaseAudioVoiceEngine::allocateNewSubmix(bool mainOut, IAudioSubmixCallback* cb, int busId) {
  return {new AudioSubmix(*this, cb, busId, mainOut)};
}

void BaseAudioVoiceEngine::setCallbackInterface(IAudioVoiceEngineCallback* cb) { m_engineCallback = cb; }

void BaseAudioVoiceEngine::setVolume(float vol) { m_totalVol = vol; }

bool BaseAudioVoiceEngine::enableLtRt(bool enable) {
  if (enable && m_mixInfo.m_channelMap.m_channelCount == 2 && m_mixInfo.m_channels == AudioChannelSet::Stereo)
    m_ltRtProcessing = std::make_unique<LtRtProcessing>(m_5msFrames, m_mixInfo);
  else
    m_ltRtProcessing.reset();
  return m_ltRtProcessing.operator bool();
}

const AudioVoiceEngineMixInfo& BaseAudioVoiceEngine::mixInfo() const { return m_mixInfo; }

const AudioVoiceEngineMixInfo& BaseAudioVoiceEngine::clientMixInfo() const {
  return m_ltRtProcessing ? m_ltRtProcessing->inMixInfo() : m_mixInfo;
}

} // namespace boo2