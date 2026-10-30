#pragma once
#include <cstdlib>
#include <string>

#include "UFDef.h"
#include "UFabi.h"
#include "CppDb.h"

namespace {

  /**
   * Returns the label of a Sage X3 field message, in the requested language.
   *
   * Parameters:
   * Application key: Folder;Server;Port;Version;Language (ignored).
   * Language code: FRA, ENG, SPA, ITA, BRI, POR.
   * Field name: Sage X3 field name in the form {tablename.fieldname}.
   * Message number.
   *
   * For example: TranslatedTextForCode("PRODV140;prodsrv;1940;140", "FRA", "ACONSULT.MODE", 8)
   *
   * Return: message label in the requested language, or "" when it is not found.
   *
   * https://developer.sage.com/x3/docs/latest/guides/x3-development-platform/platform-reference/getting-started/print-server-presentation-and-operation-guide?type=code#user-function-libraries-u212ltextdll-and-u212ltoword
   */
  inline int __stdcall TranslatedTextForCode(UFFn* fn) {
    uf::abi abi(fn);
    int argCount = abi.ArgCount();
    if (argCount < 4) {
      abi.SetResult("");
      spdlog::error("TranslatedTextForCode: not enough parameters: get {} expected 4", argCount);
      return EXIT_SUCCESS;
    }

    const std::string lang = abi.Text(1); // FRA, ENG, SPA, ITA, BRI, POR
    const std::string field = abi.Text(2);   // {tablename.fieldname}
    const int number = abi.Integer(3);       // message number
    spdlog::debug("TranslatedTextForCode: lang='{}' field='{}' number={}", lang, field, number);

    const std::string label = cppdb::Query(
      "TranslatedTextForCode",
      "SELECT TEXTE_0 FROM EXPLOIT.ATEXTE "
      "WHERE NOMOBJ_0 + '.' + CLEREC_0 = ? AND LAN_0 = ? AND NUMERO_0 = ?",
      { cppdb::Text(field.c_str()), cppdb::Text(lang.c_str()), cppdb::Number(number) });

    abi.SetResult(label);

    spdlog::debug("TranslatedTextForCode: -> '{}'", label);
    return EXIT_SUCCESS;
  }
}
