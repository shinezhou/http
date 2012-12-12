/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:zhouxiang@baixing.com                                         |
  +----------------------------------------------------------------------+
*/


#ifndef PHP_HTTP_H
#define PHP_HTTP_H

extern zend_module_entry http_module_entry;
#define phpext_http_ptr &http_module_entry

#ifdef PHP_WIN32
#	define PHP_HTTP_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_HTTP_API __attribute__ ((visibility("default")))
#else
#	define PHP_HTTP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif


#include <curl/curl.h>

#define ITEM_MAX 64

typedef struct {
	char* key;
	uint  key_len;
	CURL* curl;
} curl_item;

PHP_MINIT_FUNCTION(http);
PHP_MSHUTDOWN_FUNCTION(http);
PHP_RINIT_FUNCTION(http);
PHP_RSHUTDOWN_FUNCTION(http);
PHP_MINFO_FUNCTION(http);

PHP_FUNCTION(http_get);		/* http get method */
PHP_FUNCTION(http_post);	/* http post method */
PHP_FUNCTION(http_info);	/* get the latest excute error info */


/* In every utility function you add that needs to use variables 
   in php_http_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as HTTP_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define HTTP_G(v) TSRMG(http_globals_id, zend_http_globals *, v)
#else
#define HTTP_G(v) (http_globals.v)
#endif

#endif	/* PHP_HTTP_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
