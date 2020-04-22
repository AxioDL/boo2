#include "AudioSubmix.hpp"
#include "AudioVoice.hpp"
#include "AudioVoiceEngine.hpp"

#include <algorithm>

#undef min
#undef max

namespace boo2 {

AudioSubmix::AudioSubmix(BaseAudioVoiceEngine& root, IAudioSubmixCallback* cb, int busId, bool mainOut)
: ListNode<AudioSubmix, BaseAudioVoiceEngine*, IAudioSubmix>(&root), m_busId(busId), m_mainOut(mainOut), m_cb(cb) {
  if (mainOut)
    setSendLevel(m_head->m_mainSubmix.get(), 1.f, false);
}

AudioSubmix::~AudioSubmix() { m_head->m_submixesDirty = true; }

AudioSubmix*& AudioSubmix::_getHeadPtr(BaseAudioVoiceEngine* head) { return head->m_submixHead; }
std::unique_lock<std::recursive_mutex> AudioSubmix::_getHeadLock(BaseAudioVoiceEngine* head) {
  return std::unique_lock<std::recursive_mutex>{head->m_dataMutex};
}

bool AudioSubmix::_isDirectDependencyOf(AudioSubmix* send) { return m_sendGains.find(send) != m_sendGains.cend(); }

bool AudioSubmix::_mergeC3(std::list<AudioSubmix*>& output, std::vector<std::list<AudioSubmix*>>& lists) {
  for (auto outerIt = lists.begin(); outerIt != lists.cend(); ++outerIt) {
    if (outerIt->empty())
      continue;
    AudioSubmix* smx = outerIt->front();
    bool found = false;
    for (auto innerIt = lists.begin(); innerIt != lists.cend(); ++innerIt) {
      if (innerIt->empty() || outerIt == innerIt)
        continue;
      if (smx == innerIt->front()) {
        innerIt->pop_front();
        found = true;
      }
    }
    if (found) {
      outerIt->pop_front();
      output.push_back(smx);
      return true;
    }
  }
  return false;
}

std::list<AudioSubmix*> AudioSubmix::_linearizeC3() {
  std::vector<std::list<AudioSubmix*>> lists = {{}};
  if (m_head->m_submixHead)
    for (AudioSubmix& smx : *m_head->m_submixHead) {
      if (&smx == this)
        continue;
      if (smx._isDirectDependencyOf(this))
        lists[0].push_back(&smx);
    }
  lists.reserve(lists[0].size() + 1);
  for (AudioSubmix* smx : lists[0])
    lists.push_back(smx->_linearizeC3());

  std::list<AudioSubmix*> ret = {this};
  while (_mergeC3(ret, lists)) {}
  return ret;
}

void AudioSubmix::_zeroFill() {
  if (m_scratch.size())
    std::fill(m_scratch.begin(), m_scratch.end(), 0);
}

float* AudioSubmix::_getMergeBuf(size_t frames) {
  if (m_redirect)
    return m_redirect;

  size_t sampleCount = frames * m_head->clientMixInfo().m_channelMap.m_channelCount;
  if (m_scratch.size() < sampleCount)
    m_scratch.resize(sampleCount);

  return m_scratch.data();
}

size_t AudioSubmix::_pumpAndMix(size_t frames) {
  const ChannelMap& chMap = m_head->clientMixInfo().m_channelMap;
  size_t chanCount = chMap.m_channelCount;

  if (m_redirect) {
    if (m_cb && m_cb->canApplyEffect())
      m_cb->applyEffect(m_redirect, frames, chMap, m_head->mixInfo().m_sampleRate);
    m_redirect += chanCount * frames;
  } else {
    size_t sampleCount = frames * chanCount;
    if (m_scratch.size() < sampleCount)
      m_scratch.resize(sampleCount);
    if (m_cb && m_cb->canApplyEffect())
      m_cb->applyEffect(m_scratch.data(), frames, chMap, m_head->mixInfo().m_sampleRate);

    size_t curSlewFrame = m_slewFrames;
    for (auto& smx : m_sendGains) {
      curSlewFrame = m_curSlewFrame;
      AudioSubmix& sm = *reinterpret_cast<AudioSubmix*>(smx.first);
      auto it = m_scratch.begin();
      float* dataOut = sm._getMergeBuf(frames);

      for (size_t f = 0; f < frames; ++f) {
        if (m_slewFrames && curSlewFrame < m_slewFrames) {
          double t = curSlewFrame / double(m_slewFrames);
          double omt = 1.0 - t;

          for (unsigned c = 0; c < chanCount; ++c) {
            *dataOut = *dataOut + *it * (smx.second[1] * t + smx.second[0] * omt);
            ++it;
            ++dataOut;
          }

          ++curSlewFrame;
        } else {
          for (unsigned c = 0; c < chanCount; ++c) {
            *dataOut = *dataOut + *it * smx.second[1];
            ++it;
            ++dataOut;
          }
        }
      }
    }
    m_curSlewFrame += curSlewFrame;
  }

  return frames;
}

void AudioSubmix::_resetOutputSampleRate() {
  if (m_cb)
    m_cb->resetOutputSampleRate(m_head->mixInfo().m_sampleRate);
}

void AudioSubmix::resetSendLevels() {
  if (m_sendGains.empty())
    return;
  m_sendGains.clear();
  m_head->m_submixesDirty = true;
}

void AudioSubmix::setSendLevel(IAudioSubmix* submix, float level, bool slew) {
  auto search = m_sendGains.find(submix);
  if (search == m_sendGains.cend()) {
    search = m_sendGains.emplace(submix, std::array<float, 2>{1.f, 1.f}).first;
    m_head->m_submixesDirty = true;
  }

  m_slewFrames = slew ? m_head->m_5msFrames : 0;
  m_curSlewFrame = 0;

  search->second[0] = search->second[1];
  search->second[1] = level;
}

const AudioVoiceEngineMixInfo& AudioSubmix::mixInfo() const { return m_head->mixInfo(); }

double AudioSubmix::getSampleRate() const { return mixInfo().m_sampleRate; }

} // namespace boo2
