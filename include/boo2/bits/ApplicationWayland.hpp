#pragma once

#ifndef BOO2_APPLICATION_POSIX
#error May only be included by ApplicationPosix.hpp
#endif

#include "xdg-decoration-client-protocol.h"
#include "xdg-shell-client-protocol.h"

#include <poll.h>
#include <sys/mman.h>

using namespace std::literals;

namespace boo2 {

template <typename T> class WlArrayWrapper {
  wl_array* m_array;

public:
  explicit WlArrayWrapper(wl_array* array) : m_array(array) {}
  T* begin() const { return (T*)m_array->data; }
  T* end() const { return (T*)((const char*)m_array->data + m_array->size); }
};

template <class Application> class WindowWayland;

template <class App> class WaylandWindowObjs {
  static constexpr int32_t MinDim = 128;
  static constexpr int32_t MaxDim = 16384;
  friend WindowWayland<App>;
  App* m_app;
  hsh::resource_owner<hsh::surface> hshSurface;
  wl_surface* wlSurface = nullptr;
  xdg_surface* xdgSurface = nullptr;
  xdg_toplevel* xdgToplevel = nullptr;
  zxdg_toplevel_decoration_v1* xdgDecoration = nullptr;
  int32_t m_width = 128, m_height = 128;
  bool m_fullscreen = false;

  static const xdg_surface_listener SurfaceListener;
  void xdg_surface_listener_configure(xdg_surface* xdg_surface,
                                      uint32_t serial) noexcept {
    int32_t width = std::max(MinDim, std::min(MaxDim, m_width));
    int32_t height = std::max(MinDim, std::min(MaxDim, m_height));
    hshSurface.set_request_extent(hsh::extent2d(width, height));
    xdg_surface_set_window_geometry(xdgSurface, 0, 0, width, height);
    xdg_surface_ack_configure(xdg_surface, serial);
    wl_surface_commit(wlSurface);
  }
  static void xdg_surface_listener_configure(void* data,
                                             xdg_surface* xdg_surface,
                                             uint32_t serial) noexcept {
    reinterpret_cast<WaylandWindowObjs*>(data)->xdg_surface_listener_configure(
        xdg_surface, serial);
  }

  static const xdg_toplevel_listener ToplevelListener;
  void xdg_toplevel_listener_configure(xdg_toplevel* xdg_toplevel,
                                       int32_t width, int32_t height,
                                       wl_array* states) noexcept {
    // for (auto state : WlArrayWrapper<xdg_toplevel_state>(states)) {}
    if (width != 0 || height != 0) {
      m_width = width;
      m_height = height;
    }
  }
  static void xdg_toplevel_listener_configure(void* data,
                                              xdg_toplevel* xdg_toplevel,
                                              int32_t width, int32_t height,
                                              wl_array* states) noexcept {
    reinterpret_cast<WaylandWindowObjs*>(data)->xdg_toplevel_listener_configure(
        xdg_toplevel, width, height, states);
  }

  void xdg_toplevel_listener_close(xdg_toplevel* xdg_toplevel) noexcept {
    m_app->m_delegate.onQuitRequest(*m_app);
  }
  static void xdg_toplevel_listener_close(void* data,
                                          xdg_toplevel* xdg_toplevel) noexcept {
    reinterpret_cast<WaylandWindowObjs*>(data)->xdg_toplevel_listener_close(
        xdg_toplevel);
  }

  static const zxdg_toplevel_decoration_v1_listener DecorationListener;
  void zxdg_toplevel_decoration_v1_listener_configure(
      zxdg_toplevel_decoration_v1* zxdg_toplevel_decoration_v1,
      uint32_t mode) noexcept {}
  static void zxdg_toplevel_decoration_v1_listener_configure(
      void* data, zxdg_toplevel_decoration_v1* zxdg_toplevel_decoration_v1,
      uint32_t mode) noexcept {
    reinterpret_cast<WaylandWindowObjs*>(data)
        ->zxdg_toplevel_decoration_v1_listener_configure(
            zxdg_toplevel_decoration_v1, mode);
  }

public:
  explicit WaylandWindowObjs(App* a, int32_t width, int32_t height) noexcept
      : m_app(a), m_width(width), m_height(height) {
    wlSurface = wl_compositor_create_surface(a->m_registry.m_wl_compositor);
    xdgSurface =
        xdg_wm_base_get_xdg_surface(a->m_registry.m_xdg_wm_base, wlSurface);
    xdgToplevel = xdg_surface_get_toplevel(xdgSurface);
    if (a->m_registry.m_zxdg_decoration_manager_v1)
      xdgDecoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
          a->m_registry.m_zxdg_decoration_manager_v1, xdgToplevel);
    wl_surface_set_user_data(wlSurface, this);
    xdg_surface_add_listener(xdgSurface, &SurfaceListener, this);
    xdg_toplevel_add_listener(xdgToplevel, &ToplevelListener, this);
    if (xdgDecoration)
      zxdg_toplevel_decoration_v1_add_listener(xdgDecoration,
                                               &DecorationListener, this);
    xdg_toplevel_set_min_size(xdgToplevel, MinDim, MinDim);
    xdg_toplevel_set_max_size(xdgToplevel, MaxDim, MaxDim);
  }

  ~WaylandWindowObjs() noexcept {
    if (xdgDecoration)
      zxdg_toplevel_decoration_v1_destroy(xdgDecoration);
    if (xdgToplevel)
      xdg_toplevel_destroy(xdgToplevel);
    if (xdgSurface)
      xdg_surface_destroy(xdgSurface);
    if (wlSurface)
      wl_surface_destroy(wlSurface);
  }

  void toggleFullscreen() noexcept {
    if (!m_fullscreen) {
      xdg_toplevel_set_fullscreen(xdgToplevel, nullptr);
      m_fullscreen = true;
    } else {
      xdg_toplevel_unset_fullscreen(xdgToplevel);
      m_fullscreen = false;
    }
  }

  operator wl_surface*() const noexcept { return wlSurface; }
  operator xdg_surface*() const noexcept { return xdgSurface; }
  operator xdg_toplevel*() const noexcept { return xdgToplevel; }
  operator zxdg_toplevel_decoration_v1*() const noexcept {
    return xdgDecoration;
  }

  wl_surface* getWlSurface() const noexcept { return wlSurface; }
};

template <class Application>
const xdg_surface_listener WaylandWindowObjs<Application>::SurfaceListener = {
    xdg_surface_listener_configure};

template <class Application>
const xdg_toplevel_listener WaylandWindowObjs<Application>::ToplevelListener = {
    xdg_toplevel_listener_configure, xdg_toplevel_listener_close};

template <class Application>
const zxdg_toplevel_decoration_v1_listener
    WaylandWindowObjs<Application>::DecorationListener = {
        zxdg_toplevel_decoration_v1_listener_configure};

template <class Application> class WindowWayland {
  friend Application;
  WaylandWindowObjs<Application>* m_wl = nullptr;

  explicit WindowWayland(Application* parent, SystemStringView title, int x,
                         int y, int w, int h) noexcept
      : m_wl(new WaylandWindowObjs<Application>(parent, w, h)) {
    xdg_toplevel_set_title(*m_wl, title.data());
  }

  bool createSurface(vk::UniqueSurfaceKHR&& physSurface,
                     std::function<void(const hsh::extent2d&)>&& resizeLambda,
                     const hsh::extent2d& extent) noexcept {
    m_wl->hshSurface = hsh::create_surface(std::move(physSurface),
                                           std::move(resizeLambda), {}, extent);
    return m_wl->hshSurface.operator bool();
  }

  operator bool() const noexcept { return m_wl->getWlSurface() != nullptr; }
  operator wl_surface*() const noexcept { return m_wl->getWlSurface(); }

public:
  using ID = wl_surface*;
  ID getID() const noexcept { return m_wl->getWlSurface(); }
  bool operator==(ID id) const noexcept { return m_wl->getWlSurface() == id; }
  bool operator!=(ID id) const noexcept { return m_wl->getWlSurface() != id; }

  WindowWayland() noexcept = default;

  ~WindowWayland() noexcept {
    if (m_wl) {
      if (m_wl->hshSurface) {
        m_wl->hshSurface.attach_deleter_lambda(
            [objs = m_wl]() { delete objs; });
        m_wl->hshSurface = hsh::resource_owner<hsh::surface>{};
      } else {
        delete m_wl;
      }
    }
  }

  WindowWayland(WindowWayland&& other) noexcept : m_wl(other.m_wl) {
    other.m_wl = nullptr;
  }
  WindowWayland& operator=(WindowWayland&& other) noexcept {
    std::swap(m_wl, other.m_wl);
    return *this;
  }
  WindowWayland(const WindowWayland&) = delete;
  WindowWayland& operator=(const WindowWayland&) = delete;

  bool acquireNextImage() noexcept {
    return m_wl->hshSurface.acquire_next_image();
  }
  hsh::surface getSurface() const noexcept { return m_wl->hshSurface.get(); }
  operator hsh::surface() const noexcept { return getSurface(); }

  void show() noexcept {}

  void hide() noexcept {}

  void setTitle(SystemStringView title) noexcept {}
};

struct WaylandRegistry {
#define BOO2_WAYLAND_REGISTRY_ENTRY(interface)                                 \
  interface* m_##interface = nullptr;                                          \
  uint32_t m_##interface##_id = 0;
#include "WaylandRegistryEntries.def"

  void RegistryHandler(struct wl_registry* registry, uint32_t id,
                       const char* interface_in, uint32_t version) noexcept {
#define BOO2_WAYLAND_REGISTRY_ENTRY(interface)                                 \
  if (#interface##sv == interface_in) {                                        \
    m_##interface =                                                            \
        (interface*)wl_registry_bind(registry, id, &interface##_interface, 1); \
    m_##interface##_id = id;                                                   \
    return;                                                                    \
  }
#include "WaylandRegistryEntries.def"
  }
  static void _RegistryHandler(WaylandRegistry* data,
                               struct wl_registry* registry, uint32_t id,
                               const char* interface,
                               uint32_t version) noexcept {
    data->RegistryHandler(registry, id, interface, version);
  }

  void RegistryRemover(struct wl_registry* registry, uint32_t id) noexcept {
#define BOO2_WAYLAND_REGISTRY_ENTRY(interface)                                 \
  if (m_##interface##_id == id) {                                              \
    m_##interface = nullptr;                                                   \
    m_##interface##_id = 0;                                                    \
    return;                                                                    \
  }
#include "WaylandRegistryEntries.def"
  }
  static void _RegistryRemover(WaylandRegistry* data,
                               struct wl_registry* registry,
                               uint32_t id) noexcept {
    data->RegistryRemover(registry, id);
  }

  static const wl_registry_listener Listener;
  explicit WaylandRegistry(wl_display* display) noexcept {
    wl_registry* registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &Listener, this);
    wl_display_dispatch(display);
    wl_display_roundtrip(display);
  }
};

const wl_registry_listener WaylandRegistry::Listener = {
    (void (*)(void* data, struct wl_registry* wl_registry, uint32_t name,
              const char* interface, uint32_t version))_RegistryHandler,
    (void (*)(void* data, struct wl_registry* wl_registry,
              uint32_t name))_RegistryRemover};

template <class App> class WaylandApplicationObjs {
  friend App;
  App* m_app;
  WaylandWindowObjs<App>* m_kbWindow = nullptr;
  wl_pointer* wlPointer = nullptr;
  wl_keyboard* wlKeyboard = nullptr;
  XkbKeymap m_keymap;

  static const xdg_wm_base_listener WmBaseListener;
  void xdg_wm_base_listener_ping(xdg_wm_base* xdg_wm_base,
                                 uint32_t serial) noexcept {
    xdg_wm_base_pong(xdg_wm_base, serial);
  }
  static void xdg_wm_base_listener_ping(void* data, xdg_wm_base* xdg_wm_base,
                                        uint32_t serial) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->xdg_wm_base_listener_ping(
        xdg_wm_base, serial);
  }

  static const wl_pointer_listener PointerListener;
  void wl_pointer_listener_enter(struct wl_pointer* wl_pointer, uint32_t serial,
                                 struct wl_surface* surface,
                                 wl_fixed_t surface_x,
                                 wl_fixed_t surface_y) noexcept {}
  static void wl_pointer_listener_enter(void* data,
                                        struct wl_pointer* wl_pointer,
                                        uint32_t serial,
                                        struct wl_surface* surface,
                                        wl_fixed_t surface_x,
                                        wl_fixed_t surface_y) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->wl_pointer_listener_enter(
        wl_pointer, serial, surface, surface_x, surface_y);
  }

  void wl_pointer_listener_leave(struct wl_pointer* wl_pointer, uint32_t serial,
                                 struct wl_surface* surface) noexcept {}
  static void wl_pointer_listener_leave(void* data,
                                        struct wl_pointer* wl_pointer,
                                        uint32_t serial,
                                        struct wl_surface* surface) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->wl_pointer_listener_leave(
        wl_pointer, serial, surface);
  }

  void wl_pointer_listener_motion(struct wl_pointer* wl_pointer, uint32_t time,
                                  wl_fixed_t surface_x,
                                  wl_fixed_t surface_y) noexcept {}
  static void wl_pointer_listener_motion(void* data,
                                         struct wl_pointer* wl_pointer,
                                         uint32_t time, wl_fixed_t surface_x,
                                         wl_fixed_t surface_y) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->wl_pointer_listener_motion(
        wl_pointer, time, surface_x, surface_y);
  }

  void wl_pointer_listener_button(struct wl_pointer* wl_pointer,
                                  uint32_t serial, uint32_t time,
                                  uint32_t button, uint32_t state) noexcept {}
  static void wl_pointer_listener_button(void* data,
                                         struct wl_pointer* wl_pointer,
                                         uint32_t serial, uint32_t time,
                                         uint32_t button,
                                         uint32_t state) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->wl_pointer_listener_button(
        wl_pointer, serial, time, button, state);
  }

  void wl_pointer_listener_axis(struct wl_pointer* wl_pointer, uint32_t time,
                                uint32_t axis, wl_fixed_t value) noexcept {}
  static void wl_pointer_listener_axis(void* data,
                                       struct wl_pointer* wl_pointer,
                                       uint32_t time, uint32_t axis,
                                       wl_fixed_t value) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->wl_pointer_listener_axis(
        wl_pointer, time, axis, value);
  }

  void wl_pointer_listener_frame(struct wl_pointer* wl_pointer) noexcept {}
  static void
  wl_pointer_listener_frame(void* data,
                            struct wl_pointer* wl_pointer) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->wl_pointer_listener_frame(
        wl_pointer);
  }

  void wl_pointer_listener_axis_source(struct wl_pointer* wl_pointer,
                                       uint32_t axis_source) noexcept {}
  static void wl_pointer_listener_axis_source(void* data,
                                              struct wl_pointer* wl_pointer,
                                              uint32_t axis_source) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)
        ->wl_pointer_listener_axis_source(wl_pointer, axis_source);
  }

  void wl_pointer_listener_axis_stop(struct wl_pointer* wl_pointer,
                                     uint32_t time, uint32_t axis) noexcept {}
  static void wl_pointer_listener_axis_stop(void* data,
                                            struct wl_pointer* wl_pointer,
                                            uint32_t time,
                                            uint32_t axis) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)
        ->wl_pointer_listener_axis_stop(wl_pointer, time, axis);
  }

  void wl_pointer_listener_axis_discrete(struct wl_pointer* wl_pointer,
                                         uint32_t axis,
                                         int32_t discrete) noexcept {}
  static void wl_pointer_listener_axis_discrete(void* data,
                                                struct wl_pointer* wl_pointer,
                                                uint32_t axis,
                                                int32_t discrete) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)
        ->wl_pointer_listener_axis_discrete(wl_pointer, axis, discrete);
  }

  static const wl_keyboard_listener KeyboardListener;
  void wl_keyboard_listener_keymap(struct wl_keyboard* wl_keyboard,
                                   uint32_t format, int32_t fd,
                                   uint32_t size) noexcept {
    m_keymap.clear();

    if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
      const char* keymap =
          (char*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
      if (keymap == MAP_FAILED)
        return;

      m_keymap.setKeymap(keymap, strnlen(keymap, size));

      munmap((void*)keymap, size);
    }
  }
  static void wl_keyboard_listener_keymap(void* data,
                                          struct wl_keyboard* wl_keyboard,
                                          uint32_t format, int32_t fd,
                                          uint32_t size) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)
        ->wl_keyboard_listener_keymap(wl_keyboard, format, fd, size);
  }

  void wl_keyboard_listener_enter(struct wl_keyboard* wl_keyboard,
                                  uint32_t serial, struct wl_surface* surface,
                                  struct wl_array* keys) noexcept {
    if (auto* windowObjs =
            (WaylandWindowObjs<App>*)wl_surface_get_user_data(surface))
      m_kbWindow = windowObjs;
  }
  static void wl_keyboard_listener_enter(void* data,
                                         struct wl_keyboard* wl_keyboard,
                                         uint32_t serial,
                                         struct wl_surface* surface,
                                         struct wl_array* keys) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->wl_keyboard_listener_enter(
        wl_keyboard, serial, surface, keys);
  }

  void wl_keyboard_listener_leave(struct wl_keyboard* wl_keyboard,
                                  uint32_t serial,
                                  struct wl_surface* surface) noexcept {
    if (auto* windowObjs =
            (WaylandWindowObjs<App>*)wl_surface_get_user_data(surface))
      if (m_kbWindow == windowObjs)
        m_kbWindow = nullptr;
  }
  static void wl_keyboard_listener_leave(void* data,
                                         struct wl_keyboard* wl_keyboard,
                                         uint32_t serial,
                                         struct wl_surface* surface) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->wl_keyboard_listener_leave(
        wl_keyboard, serial, surface);
  }

  void wl_keyboard_listener_key(struct wl_keyboard* wl_keyboard,
                                uint32_t serial, uint32_t time, uint32_t key,
                                uint32_t state) noexcept {
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
      m_keymap.translateKey(
          key,
          [a = m_app, w = m_kbWindow, state](uint32_t sym, KeyModifier mods) {
            if (sym == '\r' && (mods & KeyModifier::Alt) != KeyModifier::None)
              w->toggleFullscreen();
            a->m_delegate.onUtf32Pressed(*a, w ? w->getWlSurface() : nullptr,
                                         sym, mods);
          },
          [a = m_app, w = m_kbWindow, state](Keycode code, KeyModifier mods) {
            a->m_delegate.onSpecialKeyPressed(
                *a, w ? w->getWlSurface() : nullptr, code, mods);
          });
    else
      m_keymap.translateKey(
          key,
          [a = m_app, w = m_kbWindow, state](uint32_t sym, KeyModifier mods) {
            a->m_delegate.onUtf32Released(*a, w ? w->getWlSurface() : nullptr,
                                          sym, mods);
          },
          [a = m_app, w = m_kbWindow, state](Keycode code, KeyModifier mods) {
            a->m_delegate.onSpecialKeyReleased(
                *a, w ? w->getWlSurface() : nullptr, code, mods);
          });
  }
  static void wl_keyboard_listener_key(void* data,
                                       struct wl_keyboard* wl_keyboard,
                                       uint32_t serial, uint32_t time,
                                       uint32_t key, uint32_t state) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)->wl_keyboard_listener_key(
        wl_keyboard, serial, time, key, state);
  }

  void wl_keyboard_listener_modifiers(struct wl_keyboard* wl_keyboard,
                                      uint32_t serial, uint32_t mods_depressed,
                                      uint32_t mods_latched,
                                      uint32_t mods_locked,
                                      uint32_t group) noexcept {
    m_keymap.updateModifiers(mods_depressed, mods_latched, mods_locked, group);
  }
  static void
  wl_keyboard_listener_modifiers(void* data, struct wl_keyboard* wl_keyboard,
                                 uint32_t serial, uint32_t mods_depressed,
                                 uint32_t mods_latched, uint32_t mods_locked,
                                 uint32_t group) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)
        ->wl_keyboard_listener_modifiers(wl_keyboard, serial, mods_depressed,
                                         mods_latched, mods_locked, group);
  }

  void wl_keyboard_listener_repeat_info(struct wl_keyboard* wl_keyboard,
                                        int32_t rate, int32_t delay) noexcept {}
  static void wl_keyboard_listener_repeat_info(void* data,
                                               struct wl_keyboard* wl_keyboard,
                                               int32_t rate,
                                               int32_t delay) noexcept {
    reinterpret_cast<WaylandApplicationObjs*>(data)
        ->wl_keyboard_listener_repeat_info(wl_keyboard, rate, delay);
  }

public:
  explicit WaylandApplicationObjs(App* a) noexcept : m_app(a) {
    xdg_wm_base_add_listener(a->m_registry.m_xdg_wm_base, &WmBaseListener,
                             this);
    wlPointer = wl_seat_get_pointer(a->m_registry.m_wl_seat);
    wlKeyboard = wl_seat_get_keyboard(a->m_registry.m_wl_seat);

    if (wlPointer)
      wl_pointer_add_listener(wlPointer, &PointerListener, this);
    if (wlKeyboard)
      wl_keyboard_add_listener(wlKeyboard, &KeyboardListener, this);
  }

  ~WaylandApplicationObjs() noexcept {
    if (wlKeyboard)
      wl_keyboard_destroy(wlKeyboard);
    if (wlPointer)
      wl_pointer_destroy(wlPointer);
  }
};

template <class Application>
const xdg_wm_base_listener WaylandApplicationObjs<Application>::WmBaseListener =
    {xdg_wm_base_listener_ping};

template <class Application>
const wl_pointer_listener WaylandApplicationObjs<Application>::PointerListener =
    {wl_pointer_listener_enter,        wl_pointer_listener_leave,
     wl_pointer_listener_motion,       wl_pointer_listener_button,
     wl_pointer_listener_axis,         wl_pointer_listener_frame,
     wl_pointer_listener_axis_source,  wl_pointer_listener_axis_stop,
     wl_pointer_listener_axis_discrete};

template <class Application>
const wl_keyboard_listener
    WaylandApplicationObjs<Application>::KeyboardListener = {
        wl_keyboard_listener_keymap,    wl_keyboard_listener_enter,
        wl_keyboard_listener_leave,     wl_keyboard_listener_key,
        wl_keyboard_listener_modifiers, wl_keyboard_listener_repeat_info};

template <template <class, class> class Delegate>
class ApplicationWayland
    : public ApplicationPosix<ApplicationWayland<Delegate>,
                              WindowWayland<ApplicationWayland<Delegate>>,
                              Delegate> {
  friend WindowWayland<ApplicationWayland<Delegate>>;
  friend WaylandWindowObjs<ApplicationWayland<Delegate>>;
  friend WaylandApplicationObjs<ApplicationWayland<Delegate>>;
  using Window = WindowWayland<ApplicationWayland<Delegate>>;
  static logvisor::Module Log;

  wl_display* m_display;
  operator wl_display*() const noexcept { return m_display; }

  WaylandRegistry m_registry;

public:
  [[nodiscard]] Window createWindow(SystemStringView title, int x, int y, int w,
                                    int h) noexcept {
    Window window(this, title, x, y, w, h);
    if (!window)
      return {};

    auto physSurface = this->m_instance.create_phys_surface(m_display, window);
    if (!physSurface) {
      Log.report(logvisor::Error,
                 fmt("Unable to create Wayland Vulkan surface"));
      return {};
    }

    if (!this->m_device) {
      this->m_device =
          VulkanDevice(this->m_instance, *this, this->m_delegate, *physSurface);
      if (!this->m_device) {
        Log.report(logvisor::Error, fmt("No valid Vulkan devices accepted"));
        return {};
      }
      m_buildingPipelines = true;
    }

    auto windowId = window.getID();
    if (!window.createSurface(std::move(physSurface),
                              [this, windowId](const hsh::extent2d& extent) {
                                this->m_delegate.onWindowResize(*this, windowId,
                                                                extent);
                              },
                              {uint32_t(w), uint32_t(h)})) {
      Log.report(logvisor::Error,
                 fmt("Vulkan surface not compatible with existing device"));
      return {};
    }

    return window;
  }

  void quit() noexcept { m_running = false; }

private:
  bool m_running = true;
  bool m_buildingPipelines = false;

  int displayPoll(short int events) noexcept {
    int ret;
    struct pollfd pfd[1];

    pfd[0].fd = wl_display_get_fd(m_display);
    pfd[0].events = events;
    do {
      ret = poll(pfd, 1, -1);
    } while (ret == -1 && errno == EINTR);

    return ret;
  }

  int displayDispatch() noexcept {
    int ret;

    if (wl_display_prepare_read(m_display) == -1)
      return wl_display_dispatch_pending(m_display);

    while (true) {
      ret = wl_display_flush(m_display);

      if (ret != -1 || errno != EAGAIN)
        break;

      if (displayPoll(POLLOUT) == -1) {
        wl_display_cancel_read(m_display);
        return -1;
      }
    }

    /* Don't stop if flushing hits an EPIPE; continue so we can read any
     * protocol error that may have triggered it. */
    if (ret < 0 && errno != EPIPE) {
      wl_display_cancel_read(m_display);
      return -1;
    }

    // if (wl_display_poll(m_display, POLLIN) == -1) {
    //  wl_display_cancel_read(m_display);
    //  return -1;
    //}

    if (wl_display_read_events(m_display) == -1)
      return -1;

    return wl_display_dispatch_pending(m_display);
  }

  int run(int argc, SystemChar** argv) noexcept {
    if (!m_registry.m_wl_compositor) {
      Log.report(logvisor::Error,
                 fmt("Unable to obtain compositor from Wayland server"));
      return 1;
    }

    if (!m_registry.m_wl_seat) {
      Log.report(logvisor::Error,
                 fmt("Unable to obtain seat from Wayland server"));
      return 1;
    }

    if (!m_registry.m_xdg_wm_base) {
      Log.report(logvisor::Error,
                 fmt("Unable to obtain XDG WM base from Wayland server"));
      return 1;
    }

    WaylandApplicationObjs<ApplicationWayland<Delegate>> Objs(this);

    if (!this->m_instance)
      return 1;

    this->m_delegate.onAppLaunched(*this);

    while (m_running) {
      displayDispatch();
      if (this->m_device) {
        this->m_device.enter_draw_context([this]() {
          if (m_buildingPipelines)
            m_buildingPipelines = this->pumpBuildPipelines();
          else
            this->m_delegate.onAppIdle(*this);
        });
      }
    }

    this->m_delegate.onAppExiting(*this);
    return 0;
  }

  template <typename... DelegateArgs>
  explicit ApplicationWayland(wl_display* display, SystemStringView appName,
                              DelegateArgs&&... args) noexcept
      : ApplicationPosix<ApplicationWayland<Delegate>,
                         WindowWayland<ApplicationWayland<Delegate>>, Delegate>(
            appName, std::forward(args)...),
        m_display(display), m_registry(display) {}

public:
  ApplicationWayland(ApplicationWayland&&) = delete;
  ApplicationWayland& operator=(ApplicationWayland&&) = delete;
  ApplicationWayland(const ApplicationWayland&) = delete;
  ApplicationWayland& operator=(const ApplicationWayland&) = delete;

  template <typename... DelegateArgs>
  static int exec(wl_display* display, int argc, SystemChar** argv,
                  SystemStringView appName, DelegateArgs&&... args) noexcept {
    ApplicationWayland app(display, appName, std::forward(args)...);
    return app.run(argc, argv);
  }

  static void wl_log_handler(const char* format, va_list arg) {
    vfprintf(stderr, format, arg);
  }
};
template <template <class, class> class Delegate>
logvisor::Module ApplicationWayland<Delegate>::Log("boo2::ApplicationWayland");

} // namespace boo2
