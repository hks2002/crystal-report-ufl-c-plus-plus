set(SPDLOG_WCHAR_FILENAMES ON CACHE BOOL "Support wchar filenames" FORCE)

FetchContent_Declare(
	spdlog
	URL
	https://codeload.github.com/gabime/spdlog/tar.gz/refs/tags/v1.17.0
	DOWNLOAD_EXTRACT_TIMESTAMP
	TRUE
)
FetchContent_MakeAvailable(spdlog)