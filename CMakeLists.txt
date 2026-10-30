set(CMAKE_POLICY_VERSION_MINIMUM "3.5")
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CrystalUFL LANGUAGES CXX)
add_library(u212LText SHARED)

# Normally, it should start with 'u25', but 'u2l' also works.
set(OUTPUT_OUTPUT_DLL_NAME "u2l2LText_cache")

set_target_properties(u212LText
	PROPERTIES
		OUTPUT_NAME ${OUTPUT_OUTPUT_DLL_NAME}
		PREFIX ""
		SUFFIX ".dll"
)

include(FetchContent)
include(cmake/spdlog.cmake)
include(cmake/cppdb.cmake)

set(SOURCES_FILES src/u212LText.cpp src/Exports.def)
set(INCLUDE_FILES
	src/Log.h
	src/UFdef.h
	src/CppDb.h
	src/Cache.h
	src/UFabi.h
	src/Codec.h
)
set(UF_SOURCES_FILES
	src/UFs/TextForCode.h
	src/UFs/TextOfChapter.h
	src/UFs/TranslatedTextForCode.h
	src/UFs/X3TranslatedText.h
)

target_sources(u212LText
	PRIVATE
		${SOURCES_FILES}
		${UF_SOURCES_FILES}
		${INCLUDE_FILES}
)
target_include_directories(u212LText
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}/src/UFs
		${cppdb_SOURCE_DIR}
)

set(LINK_LIBRARIES kernel32 odbc32 cppdb-static spdlog::spdlog)
target_link_libraries(u212LText PRIVATE ${LINK_LIBRARIES})

target_compile_definitions(u212LText
	PRIVATE $<$<CONFIG:Debug>:UFL_DEBUG>
)

add_compile_options(/W3 /utf-8 /EHsc /LD /MT)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)