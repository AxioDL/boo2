#include "AudioVoiceEngine.hpp"

#include <logvisor/logvisor.hpp>

namespace boo2 {
static logvisor::Module Log("boo::AQS");

struct LibnxAudioVoiceEngine final : BaseAudioVoiceEngine {

  std::string getCurrentAudioOutput() const override { return ""; }

  bool setCurrentAudioOutput(const char* name) override { return true; }

  bool supportsVirtualMIDIIn() const override { return false; }

  LibnxAudioVoiceEngine() {}

  ~LibnxAudioVoiceEngine() override {}

  void pumpAndMixVoices() override {}

  std::vector<std::pair<std::string, std::string>>
  enumerateAudioOutputs() const override {
    return {};
  };

  std::vector<std::pair<std::string, std::string>>
  enumerateMIDIInputs() const override {
    return {};
  };

  std::unique_ptr<IMIDIIn>
  newVirtualMIDIIn(ReceiveFunctor&& receiver) override {
    return {};
  };

  std::unique_ptr<IMIDIOut> newVirtualMIDIOut() override { return {}; };

  std::unique_ptr<IMIDIInOut>
  newVirtualMIDIInOut(ReceiveFunctor&& receiver) override {
    return {};
  };

  std::unique_ptr<IMIDIIn> newRealMIDIIn(const char* name,
                                         ReceiveFunctor&& receiver) override {
    return {};
  };

  std::unique_ptr<IMIDIOut> newRealMIDIOut(const char* name) override {
    return {};
  };

  std::unique_ptr<IMIDIInOut>
  newRealMIDIInOut(const char* name, ReceiveFunctor&& receiver) override {
    return {};
  }

  bool useMIDILock() const override { return false; }

  size_t get5MsFrames() const override { return 0; }
};

std::unique_ptr<IAudioVoiceEngine> NewAudioVoiceEngine() {
  return std::make_unique<LibnxAudioVoiceEngine>();
}

} // namespace boo2
