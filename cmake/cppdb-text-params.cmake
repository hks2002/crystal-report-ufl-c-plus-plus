# cppdb binds every text parameter as SQL_LONGVARCHAR / SQL_WLONGVARCHAR, which
# SQL Server turns into `text` / `ntext`. Those types cannot be compared with the
# nvarchar columns of the X3 tables ("The data types nvarchar and ntext are
# incompatible in the equal to operator"), so an equality lookup cannot even be
# prepared. SQL_VARCHAR / SQL_WVARCHAR make the parameter a (n)varchar of the
# value's length instead: it compares with those columns directly and leaves
# their indexes usable. cppdb has no setting for this, so the fetched source is
# edited here, once, at configure time. Run again on the next configure it finds
# the replacement already in place and does nothing.
set(_cppdb_backend "${cppdb_SOURCE_DIR}/drivers/odbc_backend.cpp")
file(READ "${_cppdb_backend}" _cppdb_source)

string(REPLACE
	"sqltype = SQL_LONGVARCHAR;"
	"sqltype = SQL_VARCHAR;"
	_cppdb_patched
	"${_cppdb_source}"
)
string(REPLACE
	"sqltype = SQL_WLONGVARCHAR;"
	"sqltype = SQL_WVARCHAR;"
	_cppdb_patched
	"${_cppdb_patched}"
)

if(_cppdb_patched STREQUAL _cppdb_source)
	# Nothing was replaced: either an earlier configure already patched this tree,
	# or cppdb changed and the patch no longer applies to it.
	if(NOT
		_cppdb_source
		MATCHES
		"sqltype = SQL_VARCHAR;"
		OR
		NOT
		_cppdb_source
		MATCHES
		"sqltype = SQL_WVARCHAR;"
	)
		message(FATAL_ERROR
			"cppdb: unexpected text parameter types in ${_cppdb_backend}"
		)
	endif()
else()
	file(WRITE "${_cppdb_backend}" "${_cppdb_patched}")
endif()

unset(_cppdb_backend)
unset(_cppdb_source)
unset(_cppdb_patched)
