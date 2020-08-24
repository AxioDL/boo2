#pragma once

#ifdef _WIN32
#define NOMINMAX
#include <winapifamily.h>
#if defined(WINAPI_FAMILY) && WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP
#define WINDOWS_STORE 1
#else
#define WINDOWS_STORE 0
#endif

#include <windows.h>
#include <wrl/client.h>
template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
#endif

#include <string>
#include <string_view>

#define FMT_STRING_ALIAS 1
#define FMT_ENFORCE_COMPILE_STRING 1
#include <fmt/format.h>

#ifndef ENABLE_BITWISE_ENUM
#define ENABLE_BITWISE_ENUM(type)                                              \
  constexpr type operator|(type a, type b) noexcept {                          \
    using T = std::underlying_type_t<type>;                                    \
    return type(static_cast<T>(a) | static_cast<T>(b));                        \
  }                                                                            \
  constexpr type operator&(type a, type b) noexcept {                          \
    using T = std::underlying_type_t<type>;                                    \
    return type(static_cast<T>(a) & static_cast<T>(b));                        \
  }                                                                            \
  constexpr type operator^(type a, type b) noexcept {                          \
    using T = std::underlying_type_t<type>;                                    \
    return type(static_cast<T>(a) ^ static_cast<T>(b));                        \
  }                                                                            \
  constexpr type& operator|=(type& a, type b) noexcept {                       \
    using T = std::underlying_type_t<type>;                                    \
    a = type(static_cast<T>(a) | static_cast<T>(b));                           \
    return a;                                                                  \
  }                                                                            \
  constexpr type& operator&=(type& a, type b) noexcept {                       \
    using T = std::underlying_type_t<type>;                                    \
    a = type(static_cast<T>(a) & static_cast<T>(b));                           \
    return a;                                                                  \
  }                                                                            \
  constexpr type operator~(type key) noexcept {                                \
    using T = std::underlying_type_t<type>;                                    \
    return type(~static_cast<T>(key));                                         \
  }                                                                            \
  constexpr bool True(type key) noexcept {                                     \
    using T = std::underlying_type_t<type>;                                    \
    return static_cast<T>(key) != 0;                                           \
  }                                                                            \
  constexpr bool False(type key) noexcept { return !True(key); }
#endif

namespace boo2 {

#ifdef UNICODE
using SystemString = std::wstring;
using SystemStringView = std::wstring_view;
using SystemChar = wchar_t;
#ifndef TEXT
#define TEXT(txt) L##txt
#endif
#else
using SystemString = std::string;
using SystemStringView = std::string_view;
using SystemChar = char;
#ifndef TEXT
#define TEXT(txt) txt
#endif
#endif

} // namespace boo2
