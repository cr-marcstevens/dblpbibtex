AC_PREREQ([2.68])
AC_INIT([dblpbibtex], [2.0.0], [marc@marc-stevens.nl],[dblpbibtex2],[https://github.com/cr-marcstevens/dblpbibtex2])
AC_CONFIG_SRCDIR([README.md])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])


DEFAULT_CXXFLAGS="-O2"
AS_IF([test "x$CXXFLAGS" = "x"],
	[CXXFLAGS="$DEFAULT_CXXFLAGS"]
	[usedefaultcxxflags=yes],
	[usedefaultcxxflags=no])

LT_INIT

AC_LANG([C++])

AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PROG_CXX


AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AS_IF([test "x$cross_compiling" != "xyes" && test "x$usedefaultcxxflags" = "xyes" ],
	[AX_CHECK_COMPILE_FLAG([-march=native], [CXXFLAGS="$CXXFLAGS -march=native"], [])])


AX_CXX_COMPILE_STDCXX([17],[noext],[mandatory])

AC_MSG_CHECKING([for C++17 filesystem header])
CXX17FILESYSTEMHEADER=""
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <filesystem>]])],
	[CXX17FILESYSTEMHEADER="<filesystem>"
	 CXX17FILESYSTEMNAMESPACE="std::filesystem"], 
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <experimental/filesystem>]])],
		[CXX17FILESYSTEMHEADER="<experimental/filesystem>"
		 CXX17FILESYSTEMNAMESPACE="std::experimental::filesystem"
		 ])
	)
AS_IF([test "x$CXX17FILESYSTEMHEADER" = "x"], AC_MSG_ERROR([not found]), AC_MSG_RESULT($CXX17FILESYSTEMHEADER))

AC_MSG_CHECKING([for C++17 filesystem library])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include $CXX17FILESYSTEMHEADER]],[[return $CXX17FILESYSTEMNAMESPACE :: exists(".");]])],
	[AC_MSG_RESULT([not needed])],
	[LIBS="$LIBS -lstdc++fs";
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include $CXX17FILESYSTEMHEADER]],[[return $CXX17FILESYSTEMNAMESPACE :: exists(".");]])],
		[AC_MSG_RESULT([libstdc++fs])],
		[AC_MSG_FAILURE([not found])]
		)
	])

AC_DEFINE_UNQUOTED([CXX17FILESYSTEMHEADER],[$CXX17FILESYSTEMHEADER],[header file for C++17 filesystem])
AC_DEFINE_UNQUOTED([CXX17FILESYSTEMNAMESPACE],[$CXX17FILESYSTEMNAMESPACE],[namespace for C++17 filesystem])

AX_PTHREAD()

LIBCURL_CHECK_CONFIG()

LIBS="$PTHREAD_LIBS $LIBS $LIBCURL"
CPPFLAGS="$CPPFLAGS $LIBCURL_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"

AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT
