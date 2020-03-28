#pragma once

#ifndef BOO2_APPLICATION_POSIX
#error May only be included by ApplicationPosix.hpp
#endif

#include <xcb/xcb_atom.h>

namespace boo2 {

template <class Application> class WindowXcb {
  friend Application;
  hsh::resource_owner<hsh::surface> m_hshSurface;
  Application* m_parent = nullptr;
  xcb_window_t m_window = 0;

  explicit WindowXcb(Application* parent, SystemStringView title, int x, int y,
                     int w, int h) noexcept
      : m_parent(parent) {
    const struct xcb_setup_t* setup = xcb_get_setup(*m_parent);
    xcb_screen_iterator_t screen = xcb_setup_roots_iterator(setup);
    if (!screen.rem) {
      Application::Log.report(logvisor::Error,
                              fmt("No screens to create window"));
      return;
    }

    m_window = xcb_generate_id(*parent);
    if (!m_window) {
      Application::Log.report(logvisor::Error,
                              fmt("Unable to generate id for window"));
      return;
    }

    uint32_t EventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t ValueList[] = {
        screen.data->black_pixel,
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_ENTER_WINDOW |
            XCB_EVENT_MASK_LEAVE_WINDOW};
    xcb_create_window(*m_parent, XCB_COPY_FROM_PARENT, m_window,
                      screen.data->root, x, y, w, h, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen.data->root_visual,
                      EventMask, ValueList);
    xcb_change_property(*m_parent, XCB_PROP_MODE_REPLACE, m_window,
                        XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, title.size(),
                        title.data());
    xcb_change_property(*m_parent, XCB_PROP_MODE_REPLACE, m_window,
                        m_parent->m_atoms.m_WM_PROTOCOLS, 4, 32, 1,
                        &m_parent->m_atoms.m_WM_DELETE_WINDOW);

    m_parent->flushLater();
  }

  bool createSurface(
      vk::UniqueSurfaceKHR&& physSurface,
      std::function<void(const hsh::extent2d&)>&& resizeLambda) noexcept {
    m_hshSurface =
        hsh::create_surface(std::move(physSurface), std::move(resizeLambda));
    return m_hshSurface.operator bool();
  }

  operator xcb_window_t() const noexcept { return m_window; }

public:
  using ID = xcb_window_t;
  ID getID() const noexcept { return m_window; }
  bool operator==(ID id) const noexcept { return m_window == id; }
  bool operator!=(ID id) const noexcept { return m_window != id; }

  WindowXcb() noexcept = default;

  ~WindowXcb() noexcept {
    if (m_parent && m_window) {
      if (m_hshSurface) {
        xcb_connection_t* conn = *m_parent;
        xcb_window_t win = m_window;
        m_hshSurface.attach_deleter_lambda(
            [conn, win]() { xcb_destroy_window(conn, win); });
      } else {
        xcb_destroy_window(*m_parent, m_window);
      }
    }
  }

  WindowXcb(WindowXcb&& other) noexcept
      : m_hshSurface(std::move(other.m_hshSurface)), m_parent(other.m_parent),
        m_window(other.m_window) {
    other.m_parent = nullptr;
    other.m_window = 0;
  }

  WindowXcb& operator=(WindowXcb&& other) noexcept {
    std::swap(m_hshSurface, other.m_hshSurface);
    std::swap(m_parent, other.m_parent);
    std::swap(m_window, other.m_window);
    return *this;
  }

  WindowXcb(const WindowXcb&) = delete;
  WindowXcb& operator=(const WindowXcb&) = delete;

  bool acquireNextImage() noexcept { return m_hshSurface.acquire_next_image(); }
  hsh::surface getSurface() const noexcept { return m_hshSurface.get(); }
  operator hsh::surface() const noexcept { return getSurface(); }

  void setTitle(SystemStringView title) noexcept {
    xcb_change_property(*m_parent, XCB_PROP_MODE_REPLACE, m_window,
                        XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, title.size(),
                        title.data());
    m_parent->flushLater();
  }
};

struct XcbAtoms {
#define BOO2_XCB_ATOM(atomname) xcb_atom_t m_##atomname;
#include "XcbAtoms.def"
  explicit XcbAtoms(xcb_connection_t* conn) noexcept {
#define BOO2_XCB_ATOM(atomname)                                                \
  xcb_intern_atom_cookie_t atomname##_cookie =                                 \
      xcb_intern_atom(conn, 0, std::strlen(#atomname), #atomname);
#include "XcbAtoms.def"
#define BOO2_XCB_ATOM(atomname)                                                \
  if (xcb_intern_atom_reply_t* reply =                                         \
          xcb_intern_atom_reply(conn, atomname##_cookie, nullptr))             \
    m_##atomname = reply->atom;
#include "XcbAtoms.def"
  }
};

template <typename T> class XcbReplyWrapper {
  xcb_get_property_reply_t* m_reply;

public:
  XcbReplyWrapper(xcb_get_property_reply_t* reply) noexcept : m_reply(reply) {
    if (!m_reply)
      return;
    assert(sizeof(T) == m_reply->format / 8 && "reply format mismatch");
  }
  ~XcbReplyWrapper() noexcept { free(m_reply); }
  operator bool() const noexcept { return m_reply != nullptr; }
  T* begin() const noexcept { return (T*)(m_reply + 1); }
  T* end() const noexcept { return begin() + m_reply->value_len; }
};

template <template <class, class> class Delegate>
class ApplicationXcb
    : public ApplicationPosix<ApplicationXcb<Delegate>,
                              WindowXcb<ApplicationXcb<Delegate>>, Delegate> {
  friend WindowXcb<ApplicationXcb<Delegate>>;
  using Window = WindowXcb<ApplicationXcb<Delegate>>;
  static logvisor::Module Log;

  xcb_connection_t* m_conn;
  operator xcb_connection_t*() const noexcept { return m_conn; }

  XcbAtoms m_atoms;
  XkbKeymap m_keymap;
  uint8_t m_xkbBaseEvent = 0;

  void setFullscreen(xcb_window_t window, bool fs) noexcept {
    union {
      xcb_client_message_event_t event = {};
      xcb_raw_generic_event_t pad;
    };
    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.window = window;
    event.type = m_atoms.m__NET_WM_STATE;
    event.data.data32[0] = fs ? 1 : 0;
    event.data.data32[1] = m_atoms.m__NET_WM_STATE_FULLSCREEN;
    xcb_send_event(m_conn, true, window,
                   XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                       XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                   (char*)&event);
  }

  void toggleFullscreen(xcb_window_t window) noexcept {
    xcb_get_property_cookie_t cookie =
        xcb_get_property(m_conn, 0, window, m_atoms.m__NET_WM_STATE,
                         XCB_ATOM_ATOM, 0, UINT32_MAX);
    if (XcbReplyWrapper<xcb_atom_t> reply =
            xcb_get_property_reply(m_conn, cookie, nullptr)) {
      for (auto atom : reply) {
        if (atom == m_atoms.m__NET_WM_STATE_FULLSCREEN) {
          setFullscreen(window, false);
          return;
        }
      }
    }
    setFullscreen(window, true);
  }

  bool m_builtWindow = false;

public:
  [[nodiscard]] Window createWindow(SystemStringView title, int x, int y, int w,
                                    int h) noexcept {
    if (m_builtWindow)
      Log.report(logvisor::Fatal,
                 fmt("boo2 currently only supports one window"));

    Window window(this, title, x, y, w, h);
    if (!window)
      return {};

    auto physSurface = this->m_instance.create_phys_surface(m_conn, window);
    if (!physSurface) {
      Log.report(logvisor::Error, fmt("Unable to create XCB Vulkan surface"));
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
                              })) {
      Log.report(logvisor::Error,
                 fmt("Vulkan surface not compatible with existing device"));
      return {};
    }

    xcb_map_window(m_conn, window);
    flushLater();

    m_builtWindow = true;
    return window;
  }

  void quit() noexcept { m_running = false; }

private:
  bool m_running = true;
  bool m_pendingFlush = false;
  bool m_buildingPipelines = false;
  void flushLater() noexcept { m_pendingFlush = true; }

  int run(int argc, SystemChar** argv) noexcept {
    if (!this->m_instance)
      return 1;

    m_keymap.setKeymap(m_conn, &m_xkbBaseEvent);

    this->m_delegate.onAppLaunched(*this);

    while (m_running) {
      if (m_pendingFlush) {
        m_pendingFlush = false;
        xcb_flush(m_conn);
      }

      xcb_generic_event_t* event = xcb_poll_for_event(m_conn);
      if (!event) {
        if (this->m_device) {
          this->m_device.enter_draw_context([this]() {
            if (m_buildingPipelines)
              m_buildingPipelines = this->pumpBuildPipelines();
            else
              this->m_delegate.onAppIdle(*this);
          });
        }
        continue;
      }

      uint8_t opcode = event->response_type & ~0x80u;
      switch (opcode) {
      case XCB_KEY_PRESS: {
        auto* m = (xcb_key_press_event_t*)event;
        m_keymap.translateKey(
            m->detail,
            [a = this, w = m->event](uint32_t sym, KeyModifier mods) {
              if (sym == '\r' && (mods & KeyModifier::Alt) != KeyModifier::None)
                a->toggleFullscreen(w);
              a->m_delegate.onUtf32Pressed(*a, w, sym, mods);
            },
            [a = this, w = m->event](Keycode code, KeyModifier mods) {
              a->m_delegate.onSpecialKeyPressed(*a, w, code, mods);
            });
        break;
      }
      case XCB_KEY_RELEASE: {
        auto* m = (xcb_key_release_event_t*)event;
        m_keymap.translateKey(
            m->detail,
            [a = this, w = m->event](uint32_t sym, KeyModifier mods) {
              a->m_delegate.onUtf32Released(*a, w, sym, mods);
            },
            [a = this, w = m->event](Keycode code, KeyModifier mods) {
              a->m_delegate.onSpecialKeyReleased(*a, w, code, mods);
            });
        break;
      }
      case XCB_BUTTON_PRESS: {
        auto* m = (xcb_button_press_event_t*)event;
        switch (m->detail) {
#define BOO2_XBUTTON(name, xnum, wlnum)                                        \
  case xnum:                                                                   \
    this->m_delegate.onMouseDown(*this, m->event,                              \
                                 hsh::offset2d(m->event_x, m->event_y),        \
                                 MouseButton::name, m_keymap.getMods());       \
    break;
#define BOO2_XBUTTON_SCROLL(x, y, xnum)                                        \
  case xnum:                                                                   \
    this->m_delegate.onScroll(*this, m->event, hsh::offset2d(x, y),            \
                              m_keymap.getMods());                             \
    break;
#include "XButtons.def"
        default:
          break;
        }
        break;
      }
      case XCB_BUTTON_RELEASE: {
        auto* m = (xcb_button_release_event_t*)event;
        switch (m->detail) {
#define BOO2_XBUTTON(name, xnum, wlnum)                                        \
  case xnum:                                                                   \
    this->m_delegate.onMouseUp(*this, m->event,                                \
                               hsh::offset2d(m->event_x, m->event_y),          \
                               MouseButton::name, m_keymap.getMods());         \
    break;
#include "XButtons.def"
        default:
          break;
        }
        break;
      }
      case XCB_MOTION_NOTIFY: {
        auto* m = (xcb_motion_notify_event_t*)event;
        this->m_delegate.onMouseMove(*this, m->event,
                                     hsh::offset2d(m->event_x, m->event_y),
                                     m_keymap.getMods());
        break;
      }
      case XCB_ENTER_NOTIFY: {
        auto* m = (xcb_enter_notify_event_t*)event;
        if (m->mode == XCB_NOTIFY_MODE_NORMAL)
          this->m_delegate.onMouseEnter(*this, m->event,
                                        hsh::offset2d(m->event_x, m->event_y),
                                        m_keymap.getMods());
        break;
      }
      case XCB_LEAVE_NOTIFY: {
        auto* m = (xcb_leave_notify_event_t*)event;
        if (m->mode == XCB_NOTIFY_MODE_NORMAL)
          this->m_delegate.onMouseLeave(*this, m->event,
                                        hsh::offset2d(m->event_x, m->event_y),
                                        m_keymap.getMods());
        break;
      }
      case XCB_CLIENT_MESSAGE: {
        auto* m = (xcb_client_message_event_t*)event;
        if (m->data.data32[0] == m_atoms.m_WM_DELETE_WINDOW)
          this->m_delegate.onQuitRequest(*this);
        break;
      }
      default:
        if (opcode == m_xkbBaseEvent) {
          switch (event->pad0) {
          case XCB_XKB_NEW_KEYBOARD_NOTIFY:
          case XCB_XKB_MAP_NOTIFY: {
            m_keymap.setKeymap(m_conn, nullptr);
            break;
          }
          case XCB_XKB_STATE_NOTIFY: {
            m_keymap.updateModifiers((xcb_xkb_state_notify_event_t*)event);
            break;
          }
          default:
            break;
          }
        }
        break;
      }
      free(event);
    }

    this->m_delegate.onAppExiting(*this);
    return 0;
  }

  template <typename... DelegateArgs>
  explicit ApplicationXcb(xcb_connection_t* conn, SystemStringView appName,
                          DelegateArgs&&... args) noexcept
      : ApplicationPosix<ApplicationXcb<Delegate>,
                         WindowXcb<ApplicationXcb<Delegate>>, Delegate>(
            appName, std::forward<DelegateArgs>(args)...),
        m_conn(conn), m_atoms(conn) {}

public:
  ApplicationXcb(ApplicationXcb&&) = delete;
  ApplicationXcb& operator=(ApplicationXcb&&) = delete;
  ApplicationXcb(const ApplicationXcb&) = delete;
  ApplicationXcb& operator=(const ApplicationXcb&) = delete;

  template <typename... DelegateArgs>
  static int exec(xcb_connection_t* conn, int argc, SystemChar** argv,
                  SystemStringView appName, DelegateArgs&&... args) noexcept {
    ApplicationXcb app(conn, appName, std::forward<DelegateArgs>(args)...);
    return app.run(argc, argv);
  }
};
template <template <class, class> class Delegate>
logvisor::Module ApplicationXcb<Delegate>::Log("boo2::ApplicationXcb");

} // namespace boo2
