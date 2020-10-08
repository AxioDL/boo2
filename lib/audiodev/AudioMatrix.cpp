#include "AudioMatrix.hpp"
#include "AudioVoiceEngine.hpp"
#include <cstring>

namespace boo2 {

void AudioMatrixMono::setDefaultMatrixCoefficients(AudioChannelSet acSet) {
  m_curSlewFrame = 0;
  m_slewFrames = 0;
  memset(&m_coefs, 0, sizeof(m_coefs));
  switch (acSet) {
  case AudioChannelSet::Stereo:
  case AudioChannelSet::Quad:
    m_coefs.v[int(AudioChannel::FrontLeft)] = 1.0;
    m_coefs.v[int(AudioChannel::FrontRight)] = 1.0;
    break;
  case AudioChannelSet::Surround51:
  case AudioChannelSet::Surround71:
    m_coefs.v[int(AudioChannel::FrontCenter)] = 1.0;
    break;
  default:
    break;
  }
}

float* AudioMatrixMono::mixMonoSampleData(const AudioVoiceEngineMixInfo& info, const float* dataIn, float* dataOut,
                                          size_t samples) {
  const ChannelMap& chmap = info.m_channelMap;
  for (size_t s = 0; s < samples; ++s, ++dataIn) {
    if (m_slewFrames && m_curSlewFrame < m_slewFrames) {
      double t = m_curSlewFrame / double(m_slewFrames);
      double omt = 1.0 - t;

      for (unsigned c = 0; c < chmap.m_channelCount; ++c) {
        AudioChannel ch = chmap.m_channels[c];
        if (ch != AudioChannel::Unknown) {
          *dataOut = *dataOut + *dataIn * (m_coefs.v[int(ch)] * t + m_oldCoefs.v[int(ch)] * omt);
          ++dataOut;
        }
      }

      ++m_curSlewFrame;
    } else {
      for (unsigned c = 0; c < chmap.m_channelCount; ++c) {
        AudioChannel ch = chmap.m_channels[c];
        if (ch != AudioChannel::Unknown) {
          *dataOut = *dataOut + *dataIn * m_coefs.v[int(ch)];
          ++dataOut;
        }
      }
    }
  }
  return dataOut;
}

void AudioMatrixStereo::setDefaultMatrixCoefficients(AudioChannelSet acSet) {
  m_curSlewFrame = 0;
  m_slewFrames = 0;
  memset(&m_coefs, 0, sizeof(m_coefs));
  switch (acSet) {
  case AudioChannelSet::Stereo:
  case AudioChannelSet::Quad:
    m_coefs.v[int(AudioChannel::FrontLeft)][0] = 1.0;
    m_coefs.v[int(AudioChannel::FrontRight)][1] = 1.0;
    break;
  case AudioChannelSet::Surround51:
  case AudioChannelSet::Surround71:
    m_coefs.v[int(AudioChannel::FrontLeft)][0] = 1.0;
    m_coefs.v[int(AudioChannel::FrontRight)][1] = 1.0;
    break;
  default:
    break;
  }
}

float* AudioMatrixStereo::mixStereoSampleData(const AudioVoiceEngineMixInfo& info, const float* dataIn, float* dataOut,
                                              size_t frames) {
  const ChannelMap& chmap = info.m_channelMap;
  for (size_t f = 0; f < frames; ++f, dataIn += 2) {
    if (m_slewFrames && m_curSlewFrame < m_slewFrames) {
      double t = m_curSlewFrame / double(m_slewFrames);
      double omt = 1.0 - t;

      for (unsigned c = 0; c < chmap.m_channelCount; ++c) {
        AudioChannel ch = chmap.m_channels[c];
        if (ch != AudioChannel::Unknown) {
          *dataOut = *dataOut + *dataIn * (m_coefs.v[int(ch)][0] * t + m_oldCoefs.v[int(ch)][0] * omt) +
                     *dataIn * (m_coefs.v[int(ch)][1] * t + m_oldCoefs.v[int(ch)][1] * omt);
          ++dataOut;
        }
      }

      ++m_curSlewFrame;
    } else {
      for (unsigned c = 0; c < chmap.m_channelCount; ++c) {
        AudioChannel ch = chmap.m_channels[c];
        if (ch != AudioChannel::Unknown) {
          *dataOut = *dataOut + dataIn[0] * m_coefs.v[int(ch)][0] + dataIn[1] * m_coefs.v[int(ch)][1];
          ++dataOut;
        }
      }
    }
  }
  return dataOut;
}

} // namespace boo2
