#pragma once

#include "ApplicationCommon.hpp"

#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#ifdef VK_USE_PLATFORM_XCB_KHR
#include <xkbcommon/xkbcommon-x11.h>
#define explicit mexplicit
#include <xcb/xkb.h>
#undef explicit
#endif
#include <xkbcommon/xkbcommon.h>

namespace boo2 {

class XkbKeymap {
  xkb_context* m_xkbCtx = nullptr;
  xkb_keymap* m_keymap = nullptr;
  xkb_state* m_keystate = nullptr;
  xkb_mod_index_t m_ctrlIdx = 0, m_altIdx = 0, m_shiftIdx = 0;

  void _getMods(KeyModifier& mods) noexcept {
    mods |= xkb_state_mod_index_is_active(m_keystate, m_ctrlIdx,
                                          XKB_STATE_MODS_DEPRESSED) == 1
                ? KeyModifier::Control
                : KeyModifier::None;
    mods |= xkb_state_mod_index_is_active(m_keystate, m_altIdx,
                                          XKB_STATE_MODS_DEPRESSED) == 1
                ? KeyModifier::Alt
                : KeyModifier::None;
    mods |= xkb_state_mod_index_is_active(m_keystate, m_shiftIdx,
                                          XKB_STATE_MODS_DEPRESSED) == 1
                ? KeyModifier::Shift
                : KeyModifier::None;
  }

public:
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  void setKeymap(const char* keymap, size_t size) noexcept {
    clear();

    m_keymap = xkb_keymap_new_from_buffer(
        m_xkbCtx, keymap, strnlen(keymap, size), XKB_KEYMAP_FORMAT_TEXT_V1,
        XKB_KEYMAP_COMPILE_NO_FLAGS);

    if (m_keymap) {
      m_ctrlIdx = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_CTRL);
      m_altIdx = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_ALT);
      m_shiftIdx = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_SHIFT);
      m_keystate = xkb_state_new(m_keymap);
    }
  }
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
  void setKeymap(xcb_connection_t* connection, uint8_t* baseEvent) noexcept {
    clear();

    if (baseEvent) {
      xkb_x11_setup_xkb_extension(connection, XKB_X11_MIN_MAJOR_XKB_VERSION,
                                  XKB_X11_MIN_MINOR_XKB_VERSION,
                                  XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr,
                                  nullptr, baseEvent, nullptr);
      constexpr uint16_t eventMask = XCB_XKB_EVENT_TYPE_STATE_NOTIFY |
                                     XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
                                     XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY;
      constexpr uint16_t maps =
          XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS |
          XCB_XKB_MAP_PART_MODIFIER_MAP | XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
          XCB_XKB_MAP_PART_KEY_ACTIONS | XCB_XKB_MAP_PART_KEY_BEHAVIORS |
          XCB_XKB_MAP_PART_VIRTUAL_MODS | XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;
      xcb_xkb_select_events_aux(connection, XCB_XKB_ID_USE_CORE_KBD, eventMask,
                                0, eventMask, maps, maps, nullptr);
    }

    int32_t deviceId = xkb_x11_get_core_keyboard_device_id(connection);

    m_keymap = xkb_x11_keymap_new_from_device(m_xkbCtx, connection, deviceId,
                                              XKB_KEYMAP_COMPILE_NO_FLAGS);

    if (m_keymap) {
      m_ctrlIdx = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_CTRL);
      m_altIdx = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_ALT);
      m_shiftIdx = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_SHIFT);
      m_keystate =
          xkb_x11_state_new_from_device(m_keymap, connection, deviceId);
    }
  }
#endif

  void clear() noexcept {
    if (m_keystate) {
      xkb_state_unref(m_keystate);
      m_keystate = nullptr;
    }
    if (m_keymap) {
      xkb_keymap_unref(m_keymap);
      m_keymap = nullptr;
    }
  }

  void translateKey(
      uint32_t key, const std::function<void(uint32_t, KeyModifier)>& utfFunc,
      const std::function<void(Keycode, KeyModifier)>& specialFunc) noexcept {
    if (m_keystate) {
      uint32_t keysym = xkb_state_key_get_utf32(m_keystate, key);
      KeyModifier mods = KeyModifier::None;
      _getMods(mods);
      if (keysym) {
        utfFunc(keysym, mods);
      } else {
        switch (key - 8) {
#define BOO2_SPECIAL_KEYCODE(name, xkbcode)                                    \
  case xkbcode:                                                                \
    specialFunc(Keycode::name, mods);                                          \
    break;
#include "SpecialKeycodes.def"
        default:
          break;
        }
      }
    }
  }

  KeyModifier getMods() noexcept {
    KeyModifier mods = KeyModifier::None;
    if (m_keystate)
      _getMods(mods);
    return mods;
  }

  void updateModifiers(uint32_t mods_depressed, uint32_t mods_latched,
                       uint32_t mods_locked, uint32_t group) noexcept {
    if (m_keystate) {
      xkb_state_update_mask(m_keystate, mods_depressed, mods_latched,
                            mods_locked, group, group, group);
    }
  }

#ifdef VK_USE_PLATFORM_XCB_KHR
  void updateModifiers(xcb_xkb_state_notify_event_t* m) noexcept {
    if (m_keystate) {
      xkb_state_update_mask(m_keystate, m->baseMods, m->latchedMods,
                            m->lockedMods, m->baseGroup, m->latchedGroup,
                            m->lockedGroup);
    }
  }
#endif

  XkbKeymap() noexcept { m_xkbCtx = xkb_context_new(XKB_CONTEXT_NO_FLAGS); }

  ~XkbKeymap() noexcept {
    if (m_keystate)
      xkb_state_unref(m_keystate);
    if (m_keymap)
      xkb_keymap_unref(m_keymap);
    if (m_xkbCtx)
      xkb_context_unref(m_xkbCtx);
  }
};

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
      return fmt::format(FMT_STRING("{}/{}"), home, def);

    return {};
  }

public:
  XdgPaths() noexcept {
    m_config = getPath("XDG_CONFIG_HOME", ".config");
    m_data = getPath("XDG_DATA_HOME", ".local/share");
    m_cache = getPath("XDG_CACHE_HOME", ".cache");
  }

  SystemString getConfigPath(SystemStringView subPath) const noexcept {
    return fmt::format(FMT_STRING("{}/{}"), m_config, subPath);
  }

  SystemString getDataPath(SystemStringView subPath) const noexcept {
    return fmt::format(FMT_STRING("{}/{}"), m_config, subPath);
  }

  SystemString getCachePath(SystemStringView subPath) const noexcept {
    return fmt::format(FMT_STRING("{}/{}"), m_config, subPath);
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
      Log.report(logvisor::Warning,
                 FMT_STRING("Unable to open {} for reading."), path);
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
      Log.report(logvisor::Warning,
                 FMT_STRING("Unable to open {} for writing."), path);
    }
  }
};
inline logvisor::Module
    PosixPipelineCacheFileManager::Log("boo2::PosixPipelineCacheFileManager");

template <class App, class Win, template <class, class> class Delegate>
class ApplicationPosix
    : public ApplicationBase,
      public ApplicationVulkan<PosixPipelineCacheFileManager> {
protected:
  Delegate<App, Win> m_delegate;
  XdgPaths m_xdgPaths;
  PosixPipelineCacheFileManager m_pcfm{m_xdgPaths};

  template <typename... DelegateArgs>
  explicit ApplicationPosix(int argc, SystemChar** argv,
                            SystemStringView appName,
                            DelegateArgs&&... args) noexcept
      : ApplicationBase(argc, argv, appName),
        ApplicationVulkan<PosixPipelineCacheFileManager>(appName),
        m_delegate(std::forward<DelegateArgs>(args)...) {}

  bool pumpBuildPipelines() noexcept {
    return this->pumpBuildVulkanPipelines(m_pcfm, static_cast<App&>(*this),
                                          m_delegate);
  }

  ~ApplicationPosix() noexcept {
    WindowDecorations::shutdown();
  }
};

} // namespace boo2

#define BOO2_APPLICATION_POSIX
#ifdef VK_USE_PLATFORM_XCB_KHR
#include "ApplicationXcb.hpp"
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#include "ApplicationWayland.hpp"
#endif
#undef BOO2_APPLICATION_POSIX

namespace boo2 {

template <template <class, class> class Delegate> class ApplicationPosixExec {
  static logvisor::Module Log;

public:
  template <typename... DelegateArgs>
  static int exec(int argc, SystemChar** argv, SystemStringView appName,
                  DelegateArgs&&... args) noexcept {
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    wl_log_set_handler_client(ApplicationWayland<Delegate>::wl_log_handler);
    if (wl_display* display = wl_display_connect(nullptr)) {
      int ret = ApplicationWayland<Delegate>::exec(
          display, argc, argv, appName, std::forward<DelegateArgs>(args)...);
      wl_display_disconnect(display);
      return ret;
    }
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    if (xcb_connection_t* conn = xcb_connect(nullptr, nullptr)) {
      int ret = ApplicationXcb<Delegate>::exec(
          conn, argc, argv, appName, std::forward<DelegateArgs>(args)...);
      xcb_disconnect(conn);
      return ret;
    }
#endif

    Log.report(logvisor::Error,
               FMT_STRING("Unable to open connection to Wayland or X display"));
    return 1;
  }
};
template <template <class, class> class Delegate>
logvisor::Module
    ApplicationPosixExec<Delegate>::Log("boo2::ApplicationPosixExec");

} // namespace boo2
