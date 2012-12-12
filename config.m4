dnl $Id$
dnl config.m4 for extension http

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(http, for http support,
dnl Make sure that the comment is aligned:
dnl [  --with-http             Include http support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(http, whether to enable http support,
Make sure that the comment is aligned:
[  --enable-http           Enable http support])

if test "$PHP_HTTP" != "no"; then
  if test -r $PHP_HTTP/include/curl/easy.h; then
    CURL_DIR=$PHP_HTTP
  else
    AC_MSG_CHECKING(for cURL in default path)
    for i in /usr/local /usr; do
      if test -r $i/include/curl/easy.h; then
        CURL_DIR=$i
        AC_MSG_RESULT(found in $i)
        break
      fi
    done
  fi

  if test -z "$CURL_DIR"; then
    AC_MSG_RESULT(not found)
    AC_MSG_ERROR(Please reinstall the libcurl distribution -
    easy.h should be in <curl-dir>/include/curl/)
  fi

  CURL_CONFIG="curl-config"
  AC_MSG_CHECKING(for cURL 7.10.5 or greater)

  if ${CURL_DIR}/bin/curl-config --libs > /dev/null 2>&1; then
    CURL_CONFIG=${CURL_DIR}/bin/curl-config
  else
    if ${CURL_DIR}/curl-config --libs > /dev/null 2>&1; then
      CURL_CONFIG=${CURL_DIR}/curl-config
    fi
  fi

  curl_version_full=`$CURL_CONFIG --version`
  curl_version=`echo ${curl_version_full} | sed -e 's/libcurl //' | $AWK 'BEGIN { FS = "."; } { printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
  if test "$curl_version" -ge 7010005; then
    AC_MSG_RESULT($curl_version_full)
    CURL_LIBS=`$CURL_CONFIG --libs`
  else
    AC_MSG_ERROR(cURL version 7.10.5 or later is required to compile php with cURL support)
  fi

  PHP_ADD_INCLUDE($CURL_DIR/include)
  PHP_EVAL_LIBLINE($CURL_LIBS, CURL_SHARED_LIBADD)
  PHP_ADD_LIBRARY_WITH_PATH(curl, $CURL_DIR/$PHP_LIBDIR, CURL_SHARED_LIBADD)
 
  dnl Write more examples of tests here...

  dnl # --with-http -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/http.h"  # you most likely want to change this
  dnl if test -r $PHP_HTTP/$SEARCH_FOR; then # path given as parameter
  dnl   HTTP_DIR=$PHP_HTTP
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for http files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       HTTP_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$HTTP_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the http distribution])
  dnl fi

  dnl # --with-http -> add include path
  dnl PHP_ADD_INCLUDE($HTTP_DIR/include)

  dnl # --with-http -> check for lib and symbol presence
  dnl LIBNAME=http # you may want to change this
  dnl LIBSYMBOL=http # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $HTTP_DIR/lib, HTTP_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_HTTPLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong http lib version or lib not found])
  dnl ],[
  dnl   -L$HTTP_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(HTTP_SHARED_LIBADD)

  PHP_NEW_EXTENSION(http, http.c, $ext_shared)
fi
