#include "boo2/inputdev/DeviceSignature.hpp"
#include "boo2/inputdev/DeviceToken.hpp"
#include "boo2/inputdev/GenericPad.hpp"
#include "boo2/inputdev/DolphinSmashAdapter.hpp"
#include "boo2/inputdev/DualshockPad.hpp"
#include "boo2/inputdev/NintendoPowerA.hpp"
#include "boo2/inputdev/XInputPad.hpp"
#include "IHIDDevice.hpp"

namespace boo2 {

const DeviceSignature BOO_DEVICE_SIGS[] = {
    DEVICE_SIG(DolphinSmashAdapter, 0x57e, 0x337, DeviceType::USB),
    DEVICE_SIG(DualshockPad, 0x54c, 0x268, DeviceType::HID),
    DEVICE_SIG(GenericPad, 0, 0, DeviceType::HID),
    DEVICE_SIG(NintendoPowerA, 0x20D6, 0xA711, DeviceType::USB),
    DEVICE_SIG(XInputPad, 0, 0, DeviceType::XInput),
    DEVICE_SIG_SENTINEL(),
};

bool DeviceSignature::DeviceMatchToken(const DeviceToken& token, const TDeviceSignatureSet& sigSet) {
  if (token.getDeviceType() == DeviceType::HID) {
    uint64_t genPadHash = dev_typeid(GenericPad);
    bool hasGenericPad = false;
    for (const DeviceSignature* sig : sigSet) {
      if (sig->m_vid == token.getVendorId() && sig->m_pid == token.getProductId() && sig->m_type != DeviceType::HID)
        return false;
      if (sig->m_typeHash == genPadHash)
        hasGenericPad = true;
    }
    return hasGenericPad;
  }
  for (const DeviceSignature* sig : sigSet) {
    if (sig->m_vid == token.getVendorId() && sig->m_pid == token.getProductId())
      return true;
  }
  return false;
}

std::shared_ptr<IHIDDevice> IHIDDeviceNew(DeviceToken& token, const std::shared_ptr<DeviceBase>& devImp);
std::shared_ptr<DeviceBase> DeviceSignature::DeviceNew(DeviceToken& token) {
  std::shared_ptr<DeviceBase> retval;

  /* Perform signature-matching to find the appropriate device-factory */
  const DeviceSignature* foundSig = nullptr;
  const DeviceSignature* sigIter = BOO_DEVICE_SIGS;
  unsigned targetVid = token.getVendorId();
  unsigned targetPid = token.getProductId();
  while (sigIter->m_name) {
    if (sigIter->m_vid == targetVid && sigIter->m_pid == targetPid) {
      foundSig = sigIter;
      break;
    }
    ++sigIter;
  }
  if (!foundSig) {
    /* Try Generic HID devices */
    if (token.getDeviceType() == DeviceType::HID) {
      retval = std::make_shared<GenericPad>(&token);
      if (!retval)
        return nullptr;

      retval->m_hidDev = IHIDDeviceNew(token, retval);
      if (!retval->m_hidDev)
        return nullptr;
      retval->m_hidDev->_startThread();

      return retval;
    }

    return nullptr;
  }
  if (foundSig->m_type != DeviceType::None && foundSig->m_type != token.getDeviceType())
    return nullptr;

  retval = foundSig->m_factory(&token);
  if (!retval)
    return nullptr;

  retval->m_hidDev = IHIDDeviceNew(token, retval);
  if (!retval->m_hidDev)
    return nullptr;
  retval->m_hidDev->_startThread();

  return retval;
}

} // namespace boo2
