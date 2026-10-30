// ============================================================================
// Cache for the database layer (CppDb.h), in the style of Caffeine: Get asks
// whether a value is there, Put stores one.
//
// The production server answers in about 200 ms per round trip and a report asks
// for the same label once per cell, so a cached value saves a round trip on every
// cell after the first. A value lives for a day: an X3 label changes when the
// folder is patched, not while a report runs.
//
// The cache also survives process exit: every Put is appended to
// %TEMP%\U212LText-cache.txt and the file is read back the first time the cache
// is used, so a restarted print server does not have to ask the database again
// for labels it fetched a minute ago. The DLL itself lives under Program Files,
// which normal users cannot write to, hence %TEMP%. Expired entries are dropped
// when the file is read, and the file is rewritten compacted at that point, so it
// cannot grow without end.
//
// Timestamps are wall-clock (system_clock), not steady_clock: the latter counts
// from an arbitrary point such as boot, so a value stored by one process would
// mean nothing to the next.
// ============================================================================
#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

namespace cache {
  class Cache {
  public:
    static constexpr int cacheSeconds = 24 * 60 * 60; // one day
    static constexpr size_t maxValues = 8192;

    // True when `key` is stored and not expired; `value` then receives a copy.
    bool Get(const std::string& key, std::string& value) {
      std::lock_guard<std::mutex> lock(mutex_);

      const auto hit = values_.find(key);
      if (hit == values_.end()) {
        return false;
      }

      if (Clock::now() - hit->second.stored > std::chrono::seconds(cacheSeconds)) {
        values_.erase(hit); // expired: whoever asked has to load it again
        return false;
      }

      value = hit->second.value;
      return true;
    }

    // Stores `value` under `key`, replacing an older value.
    void Put(const std::string& key, const std::string& value) {
      std::lock_guard<std::mutex> lock(mutex_);

      if (values_.size() >= maxValues) {
        // 这里不能直接删第一条， 要先找第一个过期的， 如果没有过期的， 就删第一条
        auto toDelete = values_.begin();
        for (auto it = toDelete; it != values_.end(); ++it) {
          if (Clock::now() - it->second.stored > std::chrono::seconds(cacheSeconds)) {
            toDelete = it;
            break;
          }
        }
        values_.erase(toDelete); // evict one, keep the bound
      }

      const Clock::time_point now = Clock::now();
      values_.insert_or_assign(key, Entry{ value, now });
    }

    void Load() {
      path_ = CacheFileName();
      spdlog::trace("Loading cache from {}", Shown(path_));
      if (path_.empty()) {
        spdlog::error("Cache path is empty");
        return;
      }

      std::ifstream in(path_, std::ios::in | std::ios::binary);
      if (!in) {
        return;
      }

      spdlog::trace("Cache loaded from {}", Shown(path_));
      const Clock::time_point now = Clock::now();
      size_t lines = 0;
      std::string line;

      while (std::getline(in, line) && values_.size() < maxValues) {
        ++lines;
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }

        const size_t first = line.find('\t');
        const size_t second = first == std::string::npos ? std::string::npos : line.find('\t', first + 1);
        if (second == std::string::npos) {
          continue;
        }

        long long seconds = 0;
        try {
          seconds = std::stoll(line.substr(second + 1));
        }
        catch (...) {
          continue;
        }

        const Clock::time_point stored = Clock::time_point(std::chrono::seconds(seconds));
        if (now - stored > std::chrono::seconds(cacheSeconds)) {
          continue; // expired while the process was not running
        }

        values_.insert_or_assign(Unescape(line.substr(0, first)),
          Entry{ Unescape(line.substr(first + 1, second - first - 1)), stored });
      }
      in.close();

      if (lines > values_.size()) {
        Save(); // dropped something (expired or over the bound): keep the file small
      }
    }

    // Rewrites the file with the values currently held.
    void Save() {
      if (path_.empty()) {
        spdlog::error("Cache path is empty");
        return;
      }
      std::lock_guard<std::mutex> lock(mutex_);

      spdlog::trace("Saving cache to {}", Shown(path_));
      const std::wstring tmp = path_ + L".tmp";
      {
        std::ofstream out(tmp, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out) {
          return;
        }
        for (const auto& pair : values_) {
          out << Line(pair.first, pair.second.value, pair.second.stored);
        }
      }

      std::error_code ignored;
      std::filesystem::rename(tmp, path_, ignored);
      if (ignored) {
        std::filesystem::remove(path_, ignored); // Windows rename does not replace an existing file
        std::filesystem::rename(tmp, path_, ignored);
      }
      spdlog::trace("Cache saved to {}", Shown(path_));
    }

  private:
    using Clock = std::chrono::system_clock;

    struct Entry {
      std::string value;
      Clock::time_point stored;
    };

    // %TEMP%\u2l2Ltext-cache.tsv; empty when TEMP is not set, which turns
    // persistence off and leaves the cache purely in memory.
    static std::wstring CacheFileName() {
      try {
        std::filesystem::path dir = std::filesystem::temp_directory_path();
        return (dir / L"U212LText-cache.txt").wstring();
      }
      catch (...) {
        // Fall back to a relative path at the working directory
        return L"U212LText-cache.txt";
      }
    }

    // fmt refuses to mix wchar_t and char in one format call, so wide paths are
    // logged through their narrow form. Logging only: the file itself is always
    // opened with the wide name.
    static std::string Shown(const std::wstring& path) {
      return std::filesystem::path(path).string();
    }

    // One line per value: key \t value \t seconds-since-epoch.
    static std::string Escape(const std::string& s) {
      std::string out;
      out.reserve(s.size());
      for (const char c : s) {
        if (c == '\\') {
          out += "\\\\";
        }
        else if (c == '\t') {
          out += "\\t";
        }
        else if (c == '\n') {
          out += "\\n";
        }
        else if (c == '\r') {
          out += "\\r";
        }
        else {
          out += c;
        }
      }
      return out;
    }

    static std::string Unescape(const std::string& s) {
      std::string out;
      out.reserve(s.size());
      for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
          const char next = s[i + 1];
          if (next == '\\') {
            out += '\\';
            ++i;
            continue;
          }
          if (next == 't') {
            out += '\t';
            ++i;
            continue;
          }
          if (next == 'n') {
            out += '\n';
            ++i;
            continue;
          }
          if (next == 'r') {
            out += '\r';
            ++i;
            continue;
          }
        }
        out += s[i];
      }
      return out;
    }

    static std::string Line(const std::string& key, const std::string& value, Clock::time_point stored) {
      const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(stored.time_since_epoch()).count();
      return Escape(key) + '\t' + Escape(value) + '\t' + std::to_string(seconds) + '\n';
    }



    std::mutex mutex_;
    std::wstring path_;
    std::unordered_map<std::string, Entry> values_;
  };

  // The one cache, kept for the life of the process.
  inline Cache& CacheStore() {
    static Cache cache;
    return cache;
  }
} // namespace cache
