#pragma once

#include <cstdint>
#include <vector>

namespace boo2 {
class IMIDIReader;

class MIDIDecoder {
  IMIDIReader& m_out;
  uint8_t m_status = 0;

public:
  MIDIDecoder(IMIDIReader& out) : m_out(out) {}
  std::vector<uint8_t>::const_iterator receiveBytes(std::vector<uint8_t>::const_iterator begin,
                                                    std::vector<uint8_t>::const_iterator end);
};

} // namespace boo2