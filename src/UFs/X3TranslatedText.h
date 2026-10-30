#pragma once
#include <cstdlib>
#include <string>

#include "UFDef.h"
#include "UFabi.h"
#include "CppDb.h"

namespace {
  // Splits "PFOOTINV~SHOAXX~50~ ~" into its three leading fields.
  bool SplitTextKey(const std::string& key, std::string parts[3]) {
    const size_t p1 = key.find('~');
    if (p1 == std::string::npos) {
      return false;
    }
    const size_t p2 = key.find('~', p1 + 1);
    if (p2 == std::string::npos) {
      return false;
    }
    parts[0] = key.substr(0, p1);
    parts[1] = key.substr(p1 + 1, p2 - p1 - 1);
    parts[2] = key.substr(p2 + 1);
    return true;
  }
}

namespace {

  /**
   * Returns the label of a Sage X3 application text, in the requested language.
   *
   * Parameters:
   * Application key: Folder;Server;Port;Version;Language (ignored).
   * Language code: FRA, ENG, SPA, ITA, BRI, POR.
   * Text key: Nom table~Nom champ~Identifiant 1~Identifiant 2 ("~" separated).
   *
   * For example: X3TranslatedText("PRODV140;prodsrv;1940", "SPA", "PFOOTINV~SHOAXX~50~ ~")
   *
   * Return: text in the specified language, or "" when it is not found.
   *
   * https://developer.sage.com/x3/docs/latest/guides/x3-development-platform/platform-reference/getting-started/print-server-presentation-and-operation-guide?type=code#user-function-libraries-u212ltextdll-and-u212ltoword
   */
  inline int __stdcall X3TranslatedText(UFFn* fn) {
    uf::abi abi(fn);
    int argCount = abi.ArgCount();
    if (argCount < 3) {
      abi.SetResult("");
      spdlog::error("X3TranslatedText: not enough parameters: get {} expected 3", argCount);
      return EXIT_SUCCESS;
    }

    const std::string lang = abi.Text(1); // FRA, ENG, SPA, ITA, BRI, POR
    const std::string key = abi.Text(2);     // "table~field~id1~id2"

    std::string parts[3];
    if (!SplitTextKey(key, parts)) {
      spdlog::error("X3TranslatedText: bad text key '{}'", key);
      abi.SetResult("");
      return EXIT_SUCCESS;
    }
    spdlog::debug("X3TranslatedText: lang='{}' key='{}'", lang, key);

    const std::string label = cppdb::Query(
      "X3TranslatedText",
      "SELECT TEXTE_0 FROM EXPLOIT.ATEXTRA "
      "WHERE CODFIC_0 = ? AND ZONE_0 = ? AND LANGUE_0 = ? AND IDENT1_0 = ?",
      { cppdb::Text(parts[0].c_str()), cppdb::Text(parts[1].c_str()),
        cppdb::Text(lang.c_str()), cppdb::Text(parts[2].c_str()) });

    abi.SetResult(label);

    spdlog::debug("X3TranslatedText: -> '{}'", label);
    return EXIT_SUCCESS;
  }
}
