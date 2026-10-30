set(DISABLE_SQLITE ON CACHE BOOL "" FORCE)
set(DISABLE_PQ ON CACHE BOOL "" FORCE)
set(DISABLE_MYSQL ON CACHE BOOL "" FORCE)
set(DISABLE_ODBC OFF CACHE BOOL "" FORCE)
set(ODBC_BACKEND_INTERNAL ON CACHE BOOL "" FORCE)

set(
	CPPDB_WINDOWS_KITS
	"C:/Program Files (x86)/Windows Kits/10"
	CACHE
		PATH
		"Windows SDK root holding the ODBC headers and odbc32.lib"
)

if(NOT ODBC_LIB OR NOT ODBC_PATH)
	if(NOT CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
		message(FATAL_ERROR
			"cppdb: the Windows SDK version is unknown; set ODBC_LIB and ODBC_PATH"
		)
	endif()
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_cppdb_sdk_arch x64)
	else()
		set(_cppdb_sdk_arch x86)
	endif()

	find_library(ODBC_LIB
		odbc32
		PATHS
		"${CPPDB_WINDOWS_KITS}/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/${_cppdb_sdk_arch}"
		NO_DEFAULT_PATH
	)
	find_path(ODBC_PATH
		sqlext.h
		PATHS
		"${CPPDB_WINDOWS_KITS}/Include/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um"
		NO_DEFAULT_PATH
	)
	if(NOT ODBC_LIB OR NOT ODBC_PATH)
		message(FATAL_ERROR
			"cppdb: ODBC library or headers not found under '${CPPDB_WINDOWS_KITS}' (change CPPDB_WINDOWS_KITS)"
		)
	endif()
endif()

# EXCLUDE_FROM_ALL: cppdb also defines its test programs, which are not wanted here.
FetchContent_Declare(
	cppdb
	URL
	https://codeload.github.com/artyom-beilis/cppdb/zip/refs/heads/master
	DOWNLOAD_EXTRACT_TIMESTAMP
	TRUE
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cppdb)

include(${CMAKE_CURRENT_LIST_DIR}/cppdb-text-params.cmake)