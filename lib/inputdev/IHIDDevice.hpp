#pragma once

#include <memory>

#include "boo2/inputdev/DeviceBase.hpp"
#include "boo2/inputdev/DeviceToken.hpp"

#if _WIN32
#include <hidsdi.h>
#endif

namespace boo2 {

class IHIDDevice : public std::enable_shared_from_this<IHIDDevice> {
  friend class DeviceBase;
  friend struct DeviceSignature;
  virtual void _deviceDisconnected() = 0;
  virtual bool _sendUSBInterruptTransfer(const uint8_t* data, size_t length) = 0;
  virtual size_t _receiveUSBInterruptTransfer(uint8_t* data, size_t length) = 0;
#if _WIN32
#if !WINDOWS_STORE
  virtual const PHIDP_PREPARSED_DATA _getReportDescriptor() = 0;
#endif
#else
  virtual std::vector<uint8_t> _getReportDescriptor() = 0;
#endif
  virtual bool _sendHIDReport(const uint8_t* data, size_t length, HIDReportType tp, uint32_t message) = 0;
  virtual size_t _receiveHIDReport(uint8_t* data, size_t length, HIDReportType tp, uint32_t message) = 0;
  virtual void _startThread() = 0;

public:
  virtual ~IHIDDevice() = default;
};

} // namespace boo2