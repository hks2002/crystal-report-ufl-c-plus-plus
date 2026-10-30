#pragma once
#include <cstdlib>
#include <string>

#include "UFDef.h"
#include "UFabi.h"
#include "CppDb.h"

namespace {

    /**
     * Returns the label of a Sage X3 local menu message, in the requested language.
     *
     * Parameters:
     * Application key: in the form FOLDER;HOST;PORT;VERSION;LANGUAGE (ignored).
     * Language code: FRA, ENG, SPA, ITA, BRI, POR.
     * Message chapter number.
     * Message number.
     *
     * For example: TextOfChapter("app;sever;1940;140", "FRA", 2229, 8)
     *
     * Return: label in the specified language, or "" when it is not found.
     *
     * https://developer.sage.com/x3/docs/latest/guides/x3-development-platform/platform-reference/getting-started/print-server-presentation-and-operation-guide?type=code#user-function-libraries-u212ltextdll-and-u212ltoword
     */
    inline int __stdcall TextOfChapter(UFFn* fn) {
        uf::abi abi(fn);
        int argCount = abi.ArgCount();
        if (argCount < 4) {
            abi.SetResult("");
            spdlog::error("TextOfChapter: not enough parameters: get {} expected 4", argCount);
            return EXIT_SUCCESS;
        }

        const std::string lang = abi.Text(1); // FRA, ENG, SPA, ITA, BRI, POR
        const int chapter = abi.Integer(2);      // message chapter number
        const int number = abi.Integer(3);       // message number
        spdlog::debug("TextOfChapter: lang='{}' chapter={} number={}", lang, chapter, number);

        const std::string label = cppdb::Query(
            "TextOfChapter",
            "SELECT LANMES_0 FROM EXPLOIT.APLSTD "
            "WHERE LAN_0 = ? AND LANCHP_0 = ? AND LANNUM_0 = ?",
            { cppdb::Text(lang.c_str()), cppdb::Number(chapter), cppdb::Number(number) });


        abi.SetResult(label);

        spdlog::debug("TextOfChapter: -> '{}'", label);
        return EXIT_SUCCESS;
    }
}
