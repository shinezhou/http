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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_http.h"
#include "ext/standard/url.h"
#include "ext/standard/basic_functions.h"
#include "ext/standard/php_smart_str.h"

static curl_item* items[ITEM_MAX];
/* True global resources - no need for thread safety here */

static CURLcode errno = 0;
static char * errstr = NULL;

/* {{{ http_functions[]
 *
 * Every user visible function must have an entry in http_functions[].
 */
const zend_function_entry http_functions[] = {
	PHP_FE(http_get, NULL)
	PHP_FE(http_post, NULL)
	PHP_FE(http_info, NULL)
	PHP_FE_END	/* Must be the last line in http_functions[] */
};
/* }}} */

/* {{{ http_module_entry
 */
zend_module_entry http_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"http",
	http_functions,
	PHP_MINIT(http),
	PHP_MSHUTDOWN(http),
	PHP_RINIT(http),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(http),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(http),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_HTTP
ZEND_GET_MODULE(http)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("http.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_http_globals, http_globals)
    STD_PHP_INI_ENTRY("http.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_http_globals, http_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_http_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_http_init_globals(zend_http_globals *http_globals)
{
	http_globals->global_value = 0;
	http_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(http)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	int i;
	for (i=0 ; i<ITEM_MAX; i++) {
		items[i] = NULL;
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(http)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	int i;
	for (i=0; i<ITEM_MAX; i++) {
		if (items[i] != NULL) {
			pefree(items[i]->key, 1);
			curl_easy_cleanup(items[i]->curl);
		} else {
			return SUCCESS;
		}
	}
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(http)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(http)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(http)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "http support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* if the curl iterm pool is full replace the first item in the pool for the next call*/
static CURL* replace(char *key, uint key_len) {
	curl_item *first;
	first = items[0];
	if (first == NULL) return NULL;
	pefree(first->key, 1);
	first->key = key;
	first->key_len = key_len;
	return first->curl;
}


/* get the key of the url just host + port*/
static int gen_key(char *url, uint url_len, char **key, uint *key_len)
{
	php_url *p_url;
	uint len;
	p_url = php_url_parse_ex(url, url_len);
	len = strlen(p_url->host);
	char port[10];
	sprintf(port, "%d", p_url->port);
	*key_len = len + strlen(port)+1;
	*key = pemalloc(*key_len, 1);
	strcpy(*key, p_url->host);
	strcat(*key, port);
	php_url_free(p_url);
	return 0;
}

/* get the curl item of the key */
static CURL* find(char *key, uint key_len) {
	uint i;
	curl_item *item;
	for (i=0; i<ITEM_MAX; i++) {
		item = items[i];	
		if (item == NULL) {
			item = pemalloc(sizeof(curl_item), 1);
			item->key = key;
			item->key_len = key_len;
			item->curl = curl_easy_init();
			if (item->curl != NULL) {
				curl_easy_setopt(item->curl, CURLOPT_HEADER, 0);
				curl_easy_setopt(item->curl, CURLOPT_FOLLOWLOCATION, 1);
				items[i] = item;
				return item->curl;
			}
			return NULL;
		}
		if (item->key_len == key_len && (strcmp(item->key, key) == 0)) {
			return item->curl;
		}
	}
	return replace(key, key_len);	
}


static CURL* get_curl(char *url, uint url_len)
{
	char *key;
	uint key_len;
	if (gen_key(url, url_len, &key, &key_len) == FAILURE) return NULL;
	return find(key, key_len);
}


static uint get_res(void *ptr, uint size, uint nmemb, smart_str *s) 
{
	uint new_pos;
	smart_str_appendl(s, ptr, size*nmemb);
	return size*nmemb;
}

PHP_FUNCTION(http_get)
{
	char *url;
	uint url_len;
	long timeout = 1;
	CURL *curl = NULL;
	smart_str  str = {0};

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &url, &url_len, &timeout) == FAILURE) {
		RETURN_FALSE;
	}
	
	curl = get_curl(url, url_len);
	if (curl == NULL) RETURN_FALSE;
	
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);


	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_res);

	errno = curl_easy_perform(curl);
	if (errno != 0) RETURN_FALSE;
	RETURN_STRINGL(str.c, str.len, 0);
}

PHP_FUNCTION(http_post)
{
	char *url;
	uint url_len;
	long timeout = 1;
	CURL *curl = NULL;
	zval *post = NULL;
	smart_str  str = {0};
	smart_str formstr = {0};

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa|l", &url, &url_len, &post, &timeout) == FAILURE) {
		RETURN_FALSE;
	}

	curl = get_curl(url, url_len);
	if (curl == NULL) RETURN_FALSE;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

	if (php_url_encode_hash_ex(HASH_OF(post), &formstr, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, PHP_QUERY_RFC1738 TSRMLS_CC) == FAILURE) {
        if (formstr.c) {
            efree(formstr.c);
        }   
        RETURN_FALSE;
    }   
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, formstr.c);

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_res);
	errno = curl_easy_perform(curl); 
	if (formstr.c) {
		efree(formstr.c);
	}
	if (errno != 0){
		 RETURN_FALSE;
	}
	RETURN_STRINGL(str.c, str.len, 0);
}

PHP_FUNCTION(http_info)
{
	char * errstr = curl_easy_strerror(errno);
	RETURN_STRING(errstr, 1);
}
/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_http_compiled(string arg)
   Return a string to confirm that the module is compiled in */
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
