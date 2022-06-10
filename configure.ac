AC_PREREQ([2.68])
# Important: DBLPBIBTEX version needs to match with: ./version src/core.hpp
AC_INIT([dblpbibtex], [2.3], [marc@marc-stevens.nl],[dblpbibtex],[https://github.com/cr-marcstevens/dblpbibtex])
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


# test for C++17
AX_CXX_COMPILE_STDCXX([17],,[optional])
AM_CONDITIONAL([HAVE_CXX17], [test "x$HAVE_CXX17" != "x"])

# test for C++17 filesystem
AM_COND_IF([HAVE_CXX17],[
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
 AS_IF([test "x$CXX17FILESYSTEMHEADER" = "x"], AC_MSG_RESULT([not found]), AC_MSG_RESULT($CXX17FILESYSTEMHEADER))
])

# if C++17 filesystem header found then check if library is needed
# if no C++17 filesystem header found then check for Boost filesystem library instead
AS_IF([test "x$CXX17FILESYSTEMHEADER" != "x"],[

 AC_MSG_CHECKING([for C++17 filesystem library])
 AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include $CXX17FILESYSTEMHEADER]],[[return $CXX17FILESYSTEMNAMESPACE :: exists(".");]])],
	[AC_MSG_RESULT([not needed])],
	[LIBS="$LIBS -lstdc++fs";
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include $CXX17FILESYSTEMHEADER]],[[return $CXX17FILESYSTEMNAMESPACE :: exists(".");]])],
		[AC_MSG_RESULT([libstdc++fs])],
		[AC_MSG_FAILURE([not found])]
		)
	])
],[

 AC_MSG_NOTICE([No C++17 filesystem found, will try to use Boost filesystem module instead])
 AX_BOOST_BASE([1.57],
 [
  AC_MSG_NOTICE([Boost C++ libraries found, will use C++11 and Boost filesystem module])
  AX_BOOST_FILESYSTEM
  AX_BOOST_SYSTEM

  CPPFLAGS="$BOOST_CPPFLAGS $CPPFLAGS"
  LDFLAGS="$BOOST_LDFLAGS $LDFLAGS"
  LIBS="$BOOST_FILESYSTEM_LIB $BOOST_SYSTEM_LIB $LIBS"
  CXX17FILESYSTEMHEADER="<boost/filesystem.hpp>"
  CXX17FILESYSTEMNAMESPACE="boost::filesystem"
 
  AX_CXX_COMPILE_STDCXX([11],,[mandatory])
 ],[
  AC_MSG_FAILURE([No C++17 or Boost filesystem module found])
 ])

])

AC_DEFINE_UNQUOTED([CXX17FILESYSTEMHEADER],[$CXX17FILESYSTEMHEADER],[header file for C++17 filesystem])
AC_DEFINE_UNQUOTED([CXX17FILESYSTEMNAMESPACE],[$CXX17FILESYSTEMNAMESPACE],[namespace for C++17 filesystem])

LIBCURL_CHECK_CONFIG()

LIBS="$PTHREAD_LIBS $LIBS $LIBCURL"
CPPFLAGS="$CPPFLAGS $LIBCURL_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"

AC_MSG_CHECKING([if curl_mime_* is available])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <curl/curl.h>]],[[
/* test for curl_mime, otherwise use curl_formadd */
curl_global_init(CURL_GLOBAL_DEFAULT);
CURL* curl = curl_easy_init();
curl_mime* mime = nullptr;
curl_mimepart* part = nullptr;
mime = curl_mime_init(curl);
part = curl_mime_addpart(mime);
curl_mime_name(part, "test");
return 0;
]])],AC_MSG_RESULT([yes]),[
AC_MSG_RESULT([using curl_formadd instead])
AC_DEFINE([USE_CURL_FORM],[1],[Define if curl_mime_* is not available])
])

AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT
