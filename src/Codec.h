// ============================================================================
// Text encoding at the UFL boundary.
//
// Inside this DLL every string is UTF-8: that is what the database layer
// (CppDb.h, cppdb in @utf=wide mode) returns and what the log file is written in.
//
//
// CP_THREAD_ACP and CP_UTF8 are both multi-byte, so a single API call cannot go
// from one to the other: both directions pass through UTF-16, which is what
// MultiByteToWideChar / WideCharToMultiByte provide and what ToUtf8() and
// FromUtf8() wrap. The engine boundary is the only place they are used.
// ============================================================================
#pragma once

#include <stdlib.h>
#include <string.h>

#include <string>
#include <windows.h>

namespace codec {
  // Code page used by the print engine for UFL strings.
  inline constexpr UINT ansiCp = CP_THREAD_ACP;

  namespace core {
    // Bytes of code page `cp` -> UTF-16, or an empty string when the bytes are not valid in `cp`.
    inline std::wstring ToWide(UINT cp, const std::string& in) {
      if (in.empty()) {
        return std::wstring();
      }

      const int len = MultiByteToWideChar(cp, 0, in.data(), static_cast<int>(in.size()), nullptr, 0);
      if (len <= 0) {
        return std::wstring();
      }

      std::wstring out(static_cast<size_t>(len), L'\0');
      return MultiByteToWideChar(cp, 0, in.data(), static_cast<int>(in.size()), out.data(), len) == len
        ? out
        : std::wstring();
    }

    // UTF-16 -> bytes of code page `cp`; characters `cp` cannot represent become '?'.
    inline std::string FromWide(UINT cp, const std::wstring& in) {
      if (in.empty()) {
        return std::string();
      }

      const int len =
        WideCharToMultiByte(cp, 0, in.data(), static_cast<int>(in.size()), nullptr, 0, nullptr,
                            nullptr);
      if (len <= 0) {
        return std::string();
      }

      std::string out(static_cast<size_t>(len), '\0');
      WideCharToMultiByte(cp, 0, in.data(), static_cast<int>(in.size()), out.data(), len, nullptr,
                          nullptr);
      return out;
    }

    // Bytes of code page `from` -> code page `to`. `in` is returned unchanged when
    // its bytes are not valid in `from`, so nothing is lost on a bad code page.
    inline std::string Convert(UINT from, UINT to, const std::string& in) {
      if (in.empty() || from == to) {
        return in;
      }

      const std::wstring wide = ToWide(from, in);
      return wide.empty() ? in : FromWide(to, wide);
    }
  }


  // UTF-8 -> ANSI bytes for the engine. All the engine can print is '?'.
  inline std::string FromUtf8(const std::string& utf8) {
    return core::Convert(CP_UTF8, ansiCp, utf8);
  }

  // ANSI bytes from the engine -> UTF-8.
  inline std::string ToUtf8(const std::string& ansi) {
    return core::Convert(ansiCp, CP_UTF8, ansi);
  }

  // The char * the engine puts in a parameter -> UTF-8. The text is read up to
  // maxLen bytes, so a missing terminator cannot run away; NULL gives "".
  inline std::string ToUtf8(const char* ansi, size_t maxLen = 256) {
    return ansi == nullptr ? std::string() : ToUtf8(std::string(ansi, strnlen(ansi, maxLen)));
  }

  // UTF-16 -> UTF-8, for the wide Win32 names written to the (UTF-8) log.
  inline std::string ToUtf8(const std::wstring& text) {
    return core::FromWide(CP_UTF8, text);
  }

  // True when FromUtf8() loses nothing, i.e. the engine can print every character.
  inline bool Encodable(const std::string& utf8) {
    return ToUtf8(FromUtf8(utf8)) == utf8;
  }
} // namespace codec