# Sage Crystal Report UFL（C++）

This UFLs are build for Sage reports printing. With high performance, based on database connection pooling and cache.
Contains the following UFL functions:
- TextForCode
- TextOfChapter
- TranslatedTextForCode
- X3TranslatedText

1. Suggest create an ODBC DSN named "SAGE_DB"
2. Logging trace switch is HKLM\SOFTWARE\WOW6432Node\Adonix\SRVIMP\0,1,5,0\REPORT\U2LTEXT_TraceMode = OFF / ON ; Log file path is "%Temp%“\U212LText.log
4. Cache file is "%Temp%“\U212LText-cache.txt, cache time is 24 hours. It could be updated automatically. Or be deleted manually in case you changed the database and want to refresh the cache.