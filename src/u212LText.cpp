// ============================================================================
// Crystal Reports UFL
// ============================================================================
#include <windows.h>

#include <cstdint>
#include <spdlog/spdlog.h>
#include "Log.h"
#include "UFDef.h"
#include "TextForCode.h"
#include "TextOfChapter.h"
#include "TranslatedTextForCode.h"
#include "X3TranslatedText.h"
#include "CppDb.h"
#include "Cache.h"
#include "Codec.h"


// ----------------------------------------------------------------------------
// Descriptor array
// ----------------------------------------------------------------------------
static Func funcs[] = {
    { "String TextForCode (String, String, Number)", (void*)TextForCode, nullptr, 0 },
    { "String TextOfChapter (String, String, Number, Number)", (void*)TextOfChapter, nullptr, 0 },
    { "String X3TranslatedText (String, String, String)", (void*)X3TranslatedText, nullptr, 0 },
    { "String TranslatedTextForCode (String, String, String, Number)", (void*)TranslatedTextForCode, nullptr, 0 },
    { nullptr, nullptr, nullptr, 0 }, // 28-byte sentinel
};
// Templates inserted into the editor when the function is double-clicked
static const char* templates[] = { "TextForCode(appKey_will_be_ignored, fieldNameS, msgN)",
                                  "TextOfChapter(appKey_will_be_ignored, langS, chapterN, msgN)",
                                  "X3TranslatedText(appKey_will_be_ignored, langS, textS)",
                                  "TranslatedTextForCode(appKey_will_be_ignored, langS, fieldNameS, msgN)",
                                  nullptr };
// Examples shown in the function list
static const char* examples[] = { "TextForCode(appKey, fieldNameS, msgN)",
                                 "TextOfChapter(appKey, langS, chapterN, msgN)",
                                 "X3TranslatedText(appKey, langS, textS)",
                                 "TranslatedTextForCode(appKey, langS, fieldNameS, msgN)",
                                 nullptr };
// Entry array - 3 independent descriptor ptrs + sentinel.
// size is the number of real entries, excluding the trailing sentinel
// (same semantics as the u25samp1 sample).
static const uint16_t entrySize = 5;
static Entry entries[] = {
    { entrySize, (void*)funcs },     // DefStrings
    { entrySize, (void*)templates }, // Templates
    { entrySize, (void*)examples },  // Examples
    { entrySize, nullptr },           // Sentinel
};

// ----------------------------------------------------------------------------
// Exports
// ----------------------------------------------------------------------------
#define DLL_EXPORT extern "C" __declspec(dllexport)


DLL_EXPORT void* __stdcall UF5GetFunctionDefStrings(void) {
  spdlog::trace("call UF5GetFunctionDefStrings");
  return &entries[0];
}
DLL_EXPORT void* __stdcall UFGetFunctionTemplates(void) {
  spdlog::trace("call UFGetFunctionTemplates");
  return &entries[1];
}
DLL_EXPORT void* __stdcall UFGetFunctionExamples(void) {
  spdlog::trace("call UFGetFunctionExamples");
  return &entries[2];
}
DLL_EXPORT int __stdcall UF5Initialize(unsigned int version) {
  spdlog::trace("call UF5Initialize version={:04X}", version);
  cppdb::core::Pool();
  cache::CacheStore().Load();
  return 0;
}
DLL_EXPORT int __stdcall UFTerminate(void) {
  spdlog::trace("call UFTerminate");
  cppdb::core::Pool()->clear();
  cache::CacheStore().Save();
  return 0;
}
DLL_EXPORT void __stdcall UFStartJob(UFTInt32u j) {
  spdlog::trace("call UFStartJob");
  (void)j;
}
DLL_EXPORT void __stdcall UFEndJob(UFTInt32u j) {
  spdlog::trace("call UFEndJob");
  (void)j;
}
DLL_EXPORT unsigned short __stdcall UFGetVersion(void) {
  spdlog::trace("call UFGetVersion");
  return 0x0500;
}
DLL_EXPORT void __stdcall UFSetProductLocale(int a, int b) {
  spdlog::trace("call UFSetProductLocale");
}
DLL_EXPORT char* __stdcall UFErrorRecovery(void* frame, void* context) {
  spdlog::trace("call UFErrorRecovery");
  (void)frame;
  (void)context;
  static char empty[] = "\0";
  return empty;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID rv) {
  if (r == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(h);

    uf::InitLogger();
    spdlog::info("DllMain PROCESS_ATTACH");
  }
  else if (r == DLL_PROCESS_DETACH) {
    // Synchronous logger: this line reaches the file before the process goes
    // away (flush_on(info) is in effect).
    spdlog::info("DllMain PROCESS_DETACH");
    uf::ShutdownLogger();
  }
  return TRUE;
}