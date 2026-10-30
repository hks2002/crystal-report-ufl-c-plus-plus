#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex> 
#include <string>

#include <windows.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace uf {

  std::once_flag logger_once_flag;

  /**
   * @brief Check if trace logging is enabled.
   *
   * @note
   * HKLM\SOFTWARE\WOW6432Node\Adonix\SRVIMP\0,1,5,0\REPORT
   *     U2LTEXT_TraceMode = OFF / ON
   * \n
   * Log goes to %LOCALAPPDATA%\Temp\U212LText.log (per-user directory).
   * \n
   * Registry switch: only "OFF"/"0"/"FALSE"/empty are treated as off,
   * any other value is on; a missing key is also treated as off.
   */
  inline bool TraceEnabled() {
#ifdef UFL_DEBUG
    return true;
#endif

    HKEY key = nullptr;
    LONG rc = RegOpenKeyExW(
      HKEY_LOCAL_MACHINE,
      L"SOFTWARE\\WOW6432Node\\Adonix\\SRVIMP\\0,1,5,0\\REPORT", 0,
      KEY_READ | KEY_WOW64_64KEY, &key);
    if (rc != ERROR_SUCCESS) {
      spdlog::error("RegOpenKeyExW failed: {}", rc);
      return false;
    }

    wchar_t buf[32] = { 0 };
    DWORD cb = sizeof(buf) - sizeof(wchar_t);
    DWORD type = 0;
    rc = RegQueryValueExW(key, L"U2LTEXT_TraceMode", nullptr, &type, (LPBYTE)buf, &cb);
    RegCloseKey(key);

    if (rc != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
      return false;
    }

    return _wcsicmp(buf, L"OFF") != 0 && _wcsicmp(buf, L"0") != 0 &&
      _wcsicmp(buf, L"FALSE") != 0 && buf[0] != L'\0';
  }

  // Log path: <system temp dir>\U212LText.log (per-user directory)
  inline std::wstring LogFilePath() {
    try {
      std::filesystem::path dir = std::filesystem::temp_directory_path();
      return (dir / L"U212LText.log").wstring();
    }
    catch (...) {
      // Fall back to a relative path at the working directory
      return L"U212LText.log";
    }
  }

  // Create the logger and register it as spdlog's default logger; afterwards
  // spdlog::info() can be called directly. A line costs under a microsecond to
  // write, against a 200 ms database round trip, so the logger writes on the
  // calling thread: no worker thread, and nothing is lost on shutdown. When the
  // switch is off, only info and above is kept.
  inline std::shared_ptr<spdlog::logger> CreateLogger() {
    try {
      // File sink: date, module, PID, level, append mode
      auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(LogFilePath(), false);
      file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][PID=%P][%l] %v");

      auto lg = std::make_shared<spdlog::logger>("ufl", file_sink);
      lg->set_level(TraceEnabled() ? spdlog::level::trace : spdlog::level::info);
      lg->flush_on(spdlog::level::info);

      spdlog::set_default_logger(lg);
      return lg;
    }
    catch (...) {
      return nullptr;
    }
  }

  inline void InitLogger() {
    std::call_once(logger_once_flag, [] {
      return CreateLogger() != nullptr;
      });
  }

  inline void ShutdownLogger() { spdlog::shutdown(); }
}