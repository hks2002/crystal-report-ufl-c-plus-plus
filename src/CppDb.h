// ============================================================================
// Database layer: one cppdb connection pool, one result cache and one query
// helper.
//
// cppdb keeps a connection open after a query and lends it to whichever thread
// needs one next, so a login (about five round trips, ~1 s on the production
// link) is paid once for the whole process instead of once per thread. A
// cppdb::session is not thread safe: every call borrows a connection, uses it on
// the calling thread only, and hands it back when the session goes out of scope.
//
// The production server answers in about 200 ms per round trip, so the cost of a
// formula is dominated by how many round trips it makes. Results are therefore
// cached for a day (Cache.h), and each call logs the SQL it runs, noise that only
// matters while a report is being traced; no timings are kept here.
// ============================================================================
#pragma once

// windows.h first: it defines the target architecture macros that the SDK
// headers behind the ODBC driver need, and the registration API for the check of
// the data source below.
#include <windows.h>

// backend.h must precede pool.h: ref_ptr<backend::connection> is released in
// pool.h, which needs the complete type.
#include <cppdb/backend.h>
#include <cppdb/frontend.h>
#include <cppdb/pool.h>

#include <spdlog/spdlog.h>

#include <initializer_list>
#include <string>

#include "Cache.h"
#include "DbSecrets.h"

namespace cppdb {
  // A bound query parameter: a VARCHAR text or an INTEGER number.
  struct Param {
    const char* text = nullptr;
    int number = 0;
  };

  inline Param Text(const char* value) { return Param{ value, 0 }; }
  inline Param Number(int value) { return Param{ nullptr, value }; }

  namespace core {
    inline static constexpr int poolSize = 10;
    inline static bool DsnExists(const char* name) {
      const struct {
        HKEY root;
        const char* subkey;
      } locations[] = {
        { HKEY_LOCAL_MACHINE, "SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources" },
        { HKEY_CURRENT_USER, "SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources" },
      };

      for (const auto& location : locations) {
        char driver[256] = {};
        DWORD bytes = sizeof(driver);
        const LONG rc = RegGetValueA(location.root, location.subkey, name,
          RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY, nullptr, driver,
          &bytes);
        if (rc == ERROR_SUCCESS) {
          return true;
        }
      }

      return false;
    }

    // The data source the engine sets up on the print server.
    inline constexpr char dsnName[] = "SAGE_DB";

    inline constexpr char driverName[] = "SQL Server";

    // cppdb's connection string: "odbc:" picks the backend, everything without a
    // leading '@' is passed to the driver, and keys starting with '@' are cppdb
    // settings. "@utf=wide" is what keeps every string in this DLL UTF-8: cppdb
    // then calls the wide ODBC functions and converts to and from UTF-8 itself.
    // The default, "narrow", would hand back the server's ANSI code page instead
    // and lose any character that is not ASCII. "@pool_size" is how many idle
    // connections are kept; the pool opens more on demand.
    //
    // A data source is named rather than spelled out, because that is what the
    // engine installs and what an administrator maintains; when this machine has
    // none, the values above are used instead of failing every formula.
    inline static std::string ConnectionString() {
      std::string cs = "odbc:";

      if (DsnExists(dsnName)) {
        cs += "DSN=";
        cs += dsnName;
      }
      else {
        spdlog::info("data source '{}' not found: using driver '{}' server '{}' database '{}'",
          dsnName, driverName, serverName, databaseName);
        cs += "Driver={";
        cs += driverName;
        cs += "};Server=";
        cs += serverName;
        cs += ";Database=";
        cs += databaseName;
      }

      cs += ";UID=";
      cs += uid;
      cs += ";PWD=";
      cs += pwd;
      cs += ";@utf=wide;@pool_size=";
      cs += std::to_string(poolSize);
      return cs;
    }

    // The one pool, created on first use. Deliberately never destroyed: tearing
    // its ODBC handles down while the engine unloads this DLL is what unloading
    // cannot do safely, and the process exit closes them anyway.
    inline static cppdb::pool::pointer& Pool() {
      static cppdb::pool::pointer pool = [] {
        const std::string cs = ConnectionString();
        spdlog::debug("pool creating: '{}'", cs);
        return cppdb::pool::create(cs);
        }();
      return pool;
    }

    // Cache key: the caller plus every parameter value, separated by 0x1F (which
    // never appears in a caller name or in the values we look up).
    inline std::string CacheKey(const char* caller, std::initializer_list<Param> params) {
      std::string key(caller);
      key.push_back('\x1f');
      for (const Param& p : params) {
        key.append(p.text != nullptr ? p.text : std::to_string(p.number));
        key.push_back('\x1f');
      }
      return key;
    }
  } // namespace core

  // Runs `sql` (with '?' placeholders, numbered from 1) and returns the first
  // column of the first row as text: "" when there is no row, when the value is
  // NULL, and when the call failed (the failure is logged together with `caller`).
  // A result is served from the cache when the same caller already asked for the
  // same parameters less than a day ago; only a successful call is stored.
  inline std::string Query(const char* caller, const char* sql,
    std::initializer_list<Param> params) {
    const std::string key = core::CacheKey(caller, params); // caller + parameter values
    std::string value;

    if (cache::CacheStore().Get(key, value)) {
      spdlog::debug("{}: cached", caller);
      return value;
    }

    spdlog::debug("{}: '{}'", caller, sql); // what is asked of the server

    try {
      cppdb::session session(core::Pool()->open()); // an idle connection, or a login

      cppdb::statement stmt = session << sql; // prepared, and cached on that connection
      int index = 1;
      for (const Param& p : params) {
        if (p.text != nullptr) {
          stmt.bind(index, p.text);
        }
        else {
          stmt.bind(index, p.number);
        }
        ++index;
      }

      cppdb::result row = stmt.query();
      if (row.next()) {
        row.fetch(0, value);
      }
      else {
        spdlog::debug("{}: no row returned", caller);
      }
    }
    catch (const cppdb::cppdb_error& e) {
      spdlog::error("{}: query failed: {}", caller, e.what());
      return "";
    }

    cache::CacheStore().Put(key, value);
    return value;
  }
} // namespace cppdb