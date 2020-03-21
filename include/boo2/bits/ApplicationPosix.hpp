#pragma once

#include "ApplicationCommon.hpp"

#include <cstdlib>
#include <pwd.h>
#include <unistd.h>

namespace boo2 {

class XdgPaths {
  SystemString m_config;
  SystemString m_data;
  SystemString m_cache;

  static const char* getHome() noexcept {
    const char* homeDir;
    if ((homeDir = std::getenv("HOME")) && homeDir[0] == '/')
      return homeDir;

    struct passwd* pwd;
    if (!(pwd = getpwuid(getuid())) || !pwd->pw_dir || pwd->pw_dir[0] != '/')
      return nullptr;

    return pwd->pw_dir;
  }

  static SystemString getPath(const char* env, const char* def) noexcept {
    if (const char* envLookup = std::getenv(env))
      return envLookup;

    if (const char* home = getHome())
      return fmt::format(fmt("{}/{}"), home, def);

    return {};
  }

public:
  XdgPaths() noexcept {
    m_config = getPath("XDG_CONFIG_HOME", ".config");
    m_data = getPath("XDG_DATA_HOME", ".local/share");
    m_cache = getPath("XDG_CACHE_HOME", ".cache");
  }

  SystemString getConfigPath(SystemStringView subPath) const noexcept {
    return fmt::format(fmt("{}/{}"), m_config, subPath);
  }

  SystemString getDataPath(SystemStringView subPath) const noexcept {
    return fmt::format(fmt("{}/{}"), m_config, subPath);
  }

  SystemString getCachePath(SystemStringView subPath) const noexcept {
    return fmt::format(fmt("{}/{}"), m_config, subPath);
  }
};

struct PosixPipelineCacheFileManager {
  static logvisor::Module Log;
  const XdgPaths& m_paths;

  explicit PosixPipelineCacheFileManager(XdgPaths& paths) noexcept
      : m_paths(paths) {}

  std::string GetFilename(const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    std::ostringstream FileName;
    FileName << "hsh-test-pipeline-cache-";
    for (int i = 0; i < VK_UUID_SIZE; ++i)
      FileName << std::hex << unsigned(UUID[i]);
    FileName << ".bin";
    return m_paths.getCachePath(FileName.str());
  }

  template <typename Func>
  void ReadPipelineCache(Func F, const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    auto path = GetFilename(UUID);
    if (std::FILE* File = std::fopen(path.c_str(), "rb")) {
      std::fseek(File, 0, SEEK_END);
      auto Size = std::ftell(File);
      if (Size != 0) {
        std::fseek(File, 0, SEEK_SET);
        std::unique_ptr<uint8_t[]> Data(new uint8_t[Size]);
        Size = std::fread(Data.get(), 1, Size, File);
        if (Size != 0)
          F(Data.get(), Size);
      }
      std::fclose(File);
    } else {
      Log.report(logvisor::Warning, fmt("Unable to open {} for reading."),
                 path);
    }
  }

  template <typename Func>
  void WritePipelineCache(Func F, const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    auto path = GetFilename(UUID);
    if (std::FILE* File = std::fopen(path.c_str(), "wb")) {
      F([File](const uint8_t* Data, std::size_t Size) {
        std::fwrite(Data, 1, Size, File);
      });
      std::fclose(File);
    } else {
      Log.report(logvisor::Warning, fmt("Unable to open {} for writing."),
                 path);
    }
  }
};
logvisor::Module
    PosixPipelineCacheFileManager::Log("boo2::PosixPipelineCacheFileManager");

template <class App, class Win, template <class, class> class Delegate>
class ApplicationPosix : public ApplicationBase,
                         public ApplicationVulkan<PosixPipelineCacheFileManager,
                                                  Delegate<App, Win>> {
protected:
  Delegate<App, Win> m_delegate;
  XdgPaths m_xdgPaths;
  PosixPipelineCacheFileManager m_pcfm{m_xdgPaths};

  template <typename... DelegateArgs>
  explicit ApplicationPosix(SystemStringView appName,
                            DelegateArgs&&... args) noexcept
      : ApplicationBase(appName),
        ApplicationVulkan<PosixPipelineCacheFileManager, Delegate<App, Win>>(
            appName),
        m_delegate(std::forward(args)...) {}

  bool pumpBuildPipelines() noexcept {
    return this->pumpBuildVulkanPipelines(m_pcfm, m_delegate);
  }
};

} // namespace boo2

#define BOO2_APPLICATION_POSIX
#include "ApplicationXcb.hpp"
#undef BOO2_APPLICATION_POSIX

namespace boo2 {

template <template <class, class> class Delegate> class ApplicationPosixExec {
  static logvisor::Module Log;

public:
  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
    xcb_connection_t* conn = xcb_connect(nullptr, nullptr);
    if (!conn) {
      Log.report(logvisor::Error,
                 fmt("Unable to open connection to X display"));
      return 1;
    }

    int ret = ApplicationXcb<Delegate>::exec(conn, argc, argv, appName,
                                             std::forward(args)...);
    xcb_disconnect(conn);
    return ret;
  }
};
template <template <class, class> class Delegate>
logvisor::Module
    ApplicationPosixExec<Delegate>::Log("boo2::ApplicationPosixExec");

} // namespace boo2
