#include "boo2/audiodev/MIDIDecoder.hpp"

#include <algorithm>
#include <optional>

#include "boo2/audiodev/IMIDIReader.hpp"
#include "MIDICommon.hpp"


namespace boo2 {
namespace {
constexpr uint8_t clamp7(uint8_t val) { return std::clamp(val, uint8_t{0}, uint8_t{127}); }

std::optional<uint32_t> readContinuedValue(std::vector<uint8_t>::const_iterator& it,
                                           std::vector<uint8_t>::const_iterator end) {
  uint8_t a = *it++;
  uint32_t valOut = a & 0x7f;

  if ((a & 0x80) != 0) {
    if (it == end) {
      return std::nullopt;
    }
    valOut <<= 7;
    a = *it++;
    valOut |= a & 0x7f;

    if ((a & 0x80) != 0) {
      if (it == end) {
        return std::nullopt;
      }
      valOut <<= 7;
      a = *it++;
      valOut |= a & 0x7f;
    }
  }

  return valOut;
}
} // Anonymous namespace

std::vector<uint8_t>::const_iterator MIDIDecoder::receiveBytes(std::vector<uint8_t>::const_iterator begin,
                                                               std::vector<uint8_t>::const_iterator end) {
  auto it = begin;
  while (it != end) {
    uint8_t a = *it++;
    uint8_t b;
    if (a & 0x80)
      m_status = a;
    else
      it--;

    if (m_status == 0xff) {
      /* Meta events (ignored for now) */
      if (it == end)
        return begin;
      a = *it++;

      const auto length = readContinuedValue(it, end);
      it += *length;
    } else {
      uint8_t chan = m_status & 0xf;
      switch (Status(m_status & 0xf0)) {
      case Status::NoteOff: {
        if (it == end)
          return begin;
        a = *it++;
        if (it == end)
          return begin;
        b = *it++;
        m_out.noteOff(chan, clamp7(a), clamp7(b));
        break;
      }
      case Status::NoteOn: {
        if (it == end)
          return begin;
        a = *it++;
        if (it == end)
          return begin;
        b = *it++;
        m_out.noteOn(chan, clamp7(a), clamp7(b));
        break;
      }
      case Status::NotePressure: {
        if (it == end)
          return begin;
        a = *it++;
        if (it == end)
          return begin;
        b = *it++;
        m_out.notePressure(chan, clamp7(a), clamp7(b));
        break;
      }
      case Status::ControlChange: {
        if (it == end)
          return begin;
        a = *it++;
        if (it == end)
          return begin;
        b = *it++;
        m_out.controlChange(chan, clamp7(a), clamp7(b));
        break;
      }
      case Status::ProgramChange: {
        if (it == end)
          return begin;
        a = *it++;
        m_out.programChange(chan, clamp7(a));
        break;
      }
      case Status::ChannelPressure: {
        if (it == end)
          return begin;
        a = *it++;
        m_out.channelPressure(chan, clamp7(a));
        break;
      }
      case Status::PitchBend: {
        if (it == end)
          return begin;
        a = *it++;
        if (it == end)
          return begin;
        b = *it++;
        m_out.pitchBend(chan, clamp7(b) * 128 + clamp7(a));
        break;
      }
      case Status::SysEx: {
        switch (Status(m_status & 0xff)) {
        case Status::SysEx: {
          const auto len = readContinuedValue(it, end);
          if (!len || end - it < *len) {
            return begin;
          }
          m_out.sysex(&*it, *len);
          break;
        }
        case Status::TimecodeQuarterFrame: {
          if (it == end)
            return begin;
          a = *it++;
          m_out.timeCodeQuarterFrame(a >> 4 & 0x7, a & 0xf);
          break;
        }
        case Status::SongPositionPointer: {
          if (it == end)
            return begin;
          a = *it++;
          if (it == end)
            return begin;
          b = *it++;
          m_out.songPositionPointer(clamp7(b) * 128 + clamp7(a));
          break;
        }
        case Status::SongSelect: {
          if (it == end)
            return begin;
          a = *it++;
          m_out.songSelect(clamp7(a));
          break;
        }
        case Status::TuneRequest:
          m_out.tuneRequest();
          break;
        case Status::Start:
          m_out.startSeq();
          break;
        case Status::Continue:
          m_out.continueSeq();
          break;
        case Status::Stop:
          m_out.stopSeq();
          break;
        case Status::Reset:
          m_out.reset();
          break;
        case Status::SysExTerm:
        case Status::TimingClock:
        case Status::ActiveSensing:
        default:
          break;
        }
        break;
      }
      default:
        break;
      }
    }
  }
  return it;
}

} // namespace boo2