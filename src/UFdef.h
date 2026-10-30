#pragma once

#include <stddef.h>
#include <stdint.h>

// Integer type the engine passes to the job callbacks (UFStartJob / UFEndJob).
typedef uint32_t UFTInt32u;

#pragma pack(push, 1)

// Descriptor list returned by the UF5GetFunction* exports. 6 bytes, pointer at
// +2: the shipped sample u25samp1.dll stores it as "06 00 | 10 40 53 0A", i.e.
// size at +0 and the pointer packed right after it. Its array elements happen to
// sit 8 bytes apart, but only the layout above matters here: the engine is handed
// the address of one Entry and reads the pointer at +2.
struct Entry {
  uint16_t size; // number of entries the array holds
  void* ptr;     // array of descriptors
};

// One function / template / example descriptor (28 bytes).
struct Func {
  const char* sig; // "String Name (String, Number)"
  void* fn;        // implementation
  void* extra;     // unused
  uint32_t flags;  // unused
};

// One parameter: a node of the list the engine builds before it calls us.
struct Param {
  uint16_t reserved;        // +0x00, never read: not a pointer, not linkage
  const Param* next;        // +0x02
  unsigned char unused[6];  // +0x0A (x86: +0x06)
  union {
    double number;          // +0x10 (x86: +0x0C)
    const char* text;
  } value;
};

// The call itself, as the engine hands it to a UFL function: where the result
// goes, and where the parameter list starts.
struct UFFn {
  uint16_t reserved;          // +0x00
  char* result;               // +0x02
  unsigned char unused[0x14]; // up to the parameter list
  const Param* params;        // +0x1E (x86: +0x1A)
};

#pragma pack(pop)

// Every offset recovered above, checked by the compiler.
#if defined(_M_X64) || defined(__x86_64__)
static_assert(offsetof(Param, next) == 0x02, "Param::next must stay at +0x02");
static_assert(offsetof(Param, value) == 0x10, "Param::value must stay at +0x10");
static_assert(sizeof(Param) == 0x18, "Param must stay 24 bytes");

static_assert(offsetof(UFFn, result) == 0x02, "UFFn::result must stay at +0x02");
static_assert(offsetof(UFFn, params) == 0x1E, "UFFn::params must stay at +0x1E");
#else
static_assert(offsetof(Param, next) == 0x02, "Param::next must stay at +0x02");
static_assert(offsetof(Param, value) == 0x0C, "Param::value must stay at +0x0C");
static_assert(sizeof(Param) == 0x14, "Param must stay 20 bytes");

static_assert(offsetof(UFFn, result) == 0x02, "UFFn::result must stay at +0x02");
static_assert(offsetof(UFFn, params) == 0x1A, "UFFn::params must stay at +0x1A");
#endif
