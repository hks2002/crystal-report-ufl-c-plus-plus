#pragma once
#include <cstdlib>
#include <string>

#include "UFDef.h"
#include "UFabi.h"
#include "CppDb.h"

namespace {

  /**
   * Returns the label of a Sage X3 field message, in the connection language.
   *
   * Parameters:
   * Application key: Folder;Server;Port;Version;Language (ignored).
   * Field name: Sage X3 field name in the form {tablename.fieldname}.
   * Message number.
   *
   * Return: message label, or "" when it is not found.
   *
   * https://developer.sage.com/x3/docs/latest/guides/x3-development-platform/platform-reference/getting-started/print-server-presentation-and-operation-guide?type=code#user-function-libraries-u212ltextdll-and-u212ltoword
   */
  inline int __stdcall TextForCode(UFFn* fn) {
    uf::abi abi(fn);
    int argCount = abi.ArgCount();
    if (argCount < 3) {
      abi.SetResult("");
      spdlog::error("TextForCode: not enough parameters: get {} expected 3", argCount);
      return EXIT_SUCCESS;
    }

    const std::string field = abi.Text(1); // {tablename.fieldname}
    const int number = abi.Integer(2);     // message number
    spdlog::debug("TextForCode: field='{}' number={}", field, number);

    const std::string label = cppdb::Query(
      "TextForCode",
      "SELECT TEXTE_0 FROM EXPLOIT.ATEXTE "
      "WHERE NOMOBJ_0 + '.' + CLEREC_0 = ? AND LAN_0 = 'ENG' AND NUMERO_0 = ?",
      { cppdb::Text(field.c_str()), cppdb::Number(number) });

    abi.SetResult(label);

    spdlog::debug("TextForCode: -> '{}'", label);
    return EXIT_SUCCESS;
  }
}
