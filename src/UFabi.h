#pragma once

#include <stddef.h>
#include <string.h>
#include <string>

#include <spdlog/spdlog.h>

#include "Codec.h"
#include "UFdef.h"

namespace uf {
  // Bytes the host allocates for a string result (see u2l2000.dll).
  inline constexpr size_t maxReturn = 0x1FF;

  class abi {
  public:
    explicit abi(const UFFn* fn) : fn_(fn) {}

    // Number of parameters the engine passed (the list is capped at 32).
    int ArgCount() const {
      int n = 0;
      for (const Param* p = Params(); p != nullptr && n < 32; p = p->next)
        ++n;
      return n;
    }

    // Text parameter, converted from the ANSI code page to UTF-8 (see Codec.h).
    std::string Text(int index) const {
      const Param* p = At(index);
      return p == nullptr ? "" : codec::ToUtf8(p->value.text);
    }

    // Numeric parameter with the host's x100 scaling removed.
    int Integer(int index) const {
      const Param* p = At(index);
      return p == nullptr ? 0 : static_cast<int>(p->value.number / 100);
    }

    // Copies the UTF-8 result into the buffer the host already owns, converted to
    // the code page it reads (see codec::AnsiCp).
    // Overwriting fn_->result with a pointer of our own would not reach the host:
    // the field is the address of a buffer for us to fill, not a slot to assign.
    void SetResult(const std::string& utf8) const {
      if (fn_ == nullptr || fn_->result == nullptr) {
        spdlog::error("UFL: result buffer is not available");
        return;
      }

      const std::string ansi = codec::FromUtf8(utf8);
      const size_t n = ansi.size() < (maxReturn - 1) ? ansi.size() : (maxReturn - 1);
      memcpy(fn_->result, ansi.data(), n);
      fn_->result[n] = '\0';
    }

  private:
    const UFFn* fn_;

    const Param* Params() const { return fn_ == nullptr ? nullptr : fn_->params; }
    // Parameter node `index`, or nullptr when the list is shorter than that.
    const Param* At(int index) const {
      const Param* p = Params();
      for (int i = 0; i < index && p != nullptr; ++i) {
        p = p->next;
      }
      return p;
    }

  };

}
