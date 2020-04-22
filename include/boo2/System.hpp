#pragma once

#ifdef _WIN32
#include <winapifamily.h>
#if defined(WINAPI_FAMILY) && WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP
#define WINDOWS_STORE 1
#else
#define WINDOWS_STORE 0
#endif

#include <windows.h>
#include <D3Dcommon.h>
#include <wrl/client.h>
template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
template <class T>
static inline ComPtr<T>* ReferenceComPtr(ComPtr<T>& ptr) {
  return reinterpret_cast<ComPtr<T>*>(ptr.GetAddressOf());
}
#endif

#include <string>
#include <string_view>

#define FMT_STRING_ALIAS 1
#define FMT_ENFORCE_COMPILE_STRING 1
#include <fmt/format.h>

#ifndef ENABLE_BITWISE_ENUM
#define ENABLE_BITWISE_ENUM(type)                                                                                      \
  constexpr type operator|(type a, type b) noexcept {                                                                  \
    using T = std::underlying_type_t<type>;                                                                            \
    return type(static_cast<T>(a) | static_cast<T>(b));                                                                \
  }                                                                                                                    \
  constexpr type operator&(type a, type b) noexcept {                                                                  \
    using T = std::underlying_type_t<type>;                                                                            \
    return type(static_cast<T>(a) & static_cast<T>(b));                                                                \
  }                                                                                                                    \
  constexpr type& operator|=(type& a, type b) noexcept {                                                               \
    using T = std::underlying_type_t<type>;                                                                            \
    a = type(static_cast<T>(a) | static_cast<T>(b));                                                                   \
    return a;                                                                                                          \
  }                                                                                                                    \
  constexpr type& operator&=(type& a, type b) noexcept {                                                               \
    using T = std::underlying_type_t<type>;                                                                            \
    a = type(static_cast<T>(a) & static_cast<T>(b));                                                                   \
    return a;                                                                                                          \
  }                                                                                                                    \
  constexpr type operator~(type key) noexcept {                                                                        \
    using T = std::underlying_type_t<type>;                                                                            \
    return type(~static_cast<T>(key));                                                                                 \
  }                                                                                                                    \
  constexpr bool True(type key) noexcept {                                                                             \
    using T = std::underlying_type_t<type>;                                                                            \
    return static_cast<T>(key) != 0;                                                                                   \
  }                                                                                                                    \
  constexpr bool False(type key) noexcept {                                                                            \
    return !True(key);                                                                                                 \
  }
#endif

namespace boo2 {

#ifdef _WIN32
using SystemString = std::wstring;
using SystemStringView = std::wstring_view;
using SystemChar = wchar_t;
#ifndef _SYS_STR
#define _SYS_STR(val) L##val
#endif
#else
using SystemString = std::string;
using SystemStringView = std::string_view;
using SystemChar = char;
#ifndef _SYS_STR
#define _SYS_STR(val) val
#endif
#endif

#ifndef NDEBUG
#define __BooTraceArgs , const char *file, int line
#define __BooTraceArgsUse , file, line
#define __BooTraceInitializer , m_file(file), m_line(line)
#define __BooTraceFields                                                                                               \
  const char* m_file;                                                                                                  \
  int m_line;
#define BooTrace , __FILE__, __LINE__
#else
#define __BooTraceArgs
#define __BooTraceArgsUse
#define __BooTraceInitializer
#define __BooTraceFields
#define BooTrace
#endif

} // namespace boo2
