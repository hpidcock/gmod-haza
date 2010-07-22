#include "GMLuaModule.h"

#include "curl/curl.h"
#include "CThreadedCurl.h"

GMOD_MODULE(Init, Shutdown);

std::map<lua_State *, std::vector<CThreadedCurl *>> g_Curl;

namespace Curly
{
	LUA_FUNCTION(__new)
	{
		CThreadedCurl *curly = new CThreadedCurl(L);

		CAutoUnRef meta = Lua()->GetMetaTable(MT_CURLY, TYPE_CURLY);
		curly->Ref();
		Lua()->PushUserData(meta, static_cast<void *>(curly));

		return 1;
	}

	LUA_FUNCTION(__delete)
	{
		Lua()->CheckType(1, TYPE_CURLY);

		CThreadedCurl *curly = reinterpret_cast<CThreadedCurl *>(Lua()->GetUserData(1));

		if(curly == NULL)
			return 0;

		curly->UnRef();

		return 0;
	}

	LUA_FUNCTION(SetCallback)
	{
		Lua()->CheckType(1, TYPE_CURLY);
		Lua()->CheckType(2, GLua::TYPE_FUNCTION);

		CThreadedCurl *curly = reinterpret_cast<CThreadedCurl *>(Lua()->GetUserData(1));

		if(curly == NULL)
			return 0;

		curly->SetCallback(Lua()->GetReference(2));

		return 0;
	}

	LUA_FUNCTION(Perform)
	{
		Lua()->CheckType(1, TYPE_CURLY);

		CThreadedCurl *curly = reinterpret_cast<CThreadedCurl *>(Lua()->GetUserData(1));

		if(curly == NULL)
			return 0;

		Lua()->Push((float)curly->Perform());

		return 1;
	}

	LUA_FUNCTION(SetUrl)
	{
		Lua()->CheckType(1, TYPE_CURLY);
		Lua()->CheckType(2, GLua::TYPE_STRING);

		CThreadedCurl *curly = reinterpret_cast<CThreadedCurl *>(Lua()->GetUserData(1));

		if(curly == NULL)
			return 0;

		Lua()->Push((float)curly->SetUrl(Lua()->GetString(2)));

		return 1;
	}

	LUA_FUNCTION(SetOptNumber)
	{
		Lua()->CheckType(1, TYPE_CURLY);
		Lua()->CheckType(2, GLua::TYPE_NUMBER);
		Lua()->CheckType(3, GLua::TYPE_NUMBER);

		CThreadedCurl *curly = reinterpret_cast<CThreadedCurl *>(Lua()->GetUserData(1));

		if(curly == NULL)
			return 0;

		Lua()->Push((float)curly->SetOptNumber(Lua()->GetInteger(2), Lua()->GetInteger(3)));

		return 1;
	}

	LUA_FUNCTION(SetUsername)
	{
		Lua()->CheckType(1, TYPE_CURLY);
		Lua()->CheckType(2, GLua::TYPE_STRING);

		CThreadedCurl *curly = reinterpret_cast<CThreadedCurl *>(Lua()->GetUserData(1));

		if(curly == NULL)
			return 0;

		Lua()->Push((float)curly->SetUsername(Lua()->GetString(2)));

		return 1;
	}

	LUA_FUNCTION(SetPassword)
	{
		Lua()->CheckType(1, TYPE_CURLY);
		Lua()->CheckType(2, GLua::TYPE_STRING);

		CThreadedCurl *curly = reinterpret_cast<CThreadedCurl *>(Lua()->GetUserData(1));

		if(curly == NULL)
			return 0;

		Lua()->Push((float)curly->SetPassword(Lua()->GetString(2)));

		return 1;
	}

	LUA_FUNCTION(SetCookies)
	{
		Lua()->CheckType(1, TYPE_CURLY);
		Lua()->CheckType(2, GLua::TYPE_STRING);

		CThreadedCurl *curly = reinterpret_cast<CThreadedCurl *>(Lua()->GetUserData(1));

		if(curly == NULL)
			return 0;

		Lua()->Push((float)curly->SetCookies(Lua()->GetString(2)));

		return 1;
	}

	LUA_FUNCTION(SetPostFields)
	{
		Lua()->CheckType(1, TYPE_CURLY);
		Lua()->CheckType(2, GLua::TYPE_STRING);

		CThreadedCurl *curly = reinterpret_cast<CThreadedCurl *>(Lua()->GetUserData(1));

		if(curly == NULL)
			return 0;

		Lua()->Push((float)curly->SetPostFields(Lua()->GetString(2)));

		return 1;
	}

	LUA_FUNCTION(STATIC_CallbackHook)
	{
		std::vector<CThreadedCurl *> *curlList = &g_Curl[L];

		if(curlList)
		{
			std::vector<CThreadedCurl *> copy = *curlList;
			std::vector<CThreadedCurl *>::iterator itor = copy.begin();
			while(itor != copy.end())
			{
				(*itor)->InvokeCallbacks();

				itor++;
			}
		}

		return 0;
	}
}

int SetConstants(lua_State *L);

int Init(lua_State *L)
{
	if(g_Curl.empty())
	{
		curl_global_init(CURL_GLOBAL_ALL);
	}

	g_Curl[L] = std::vector<CThreadedCurl *>();


	Lua()->SetGlobal("Curly", Curly::__new);

	CAutoUnRef meta = Lua()->GetMetaTable(MT_CURLY, TYPE_CURLY);
	{
		CAutoUnRef __index = Lua()->GetNewTable();

		__index->SetMember("Perform", Curly::Perform);
		__index->SetMember("SetCallback", Curly::SetCallback);
		__index->SetMember("SetCookies", Curly::SetCookies);
		__index->SetMember("SetOptNumber", Curly::SetOptNumber);
		__index->SetMember("SetPassword", Curly::SetPassword);
		__index->SetMember("SetPostFields", Curly::SetPostFields);
		__index->SetMember("SetUrl", Curly::SetUrl);
		__index->SetMember("SetUsername", Curly::SetUsername);

		meta->SetMember("__index", __index);
	}
	meta->SetMember("__gc", Curly::__delete);


	CAutoUnRef hookSystem = Lua()->GetGlobal("hook");
	CAutoUnRef hookAdd = hookSystem->GetMember("Add");
	
	hookAdd->Push();
	Lua()->Push("Think");
	Lua()->Push("__CURLY_CALLBACKHOOK__");
	Lua()->Push(Curly::STATIC_CallbackHook);

	Lua()->Call(3, 0);

	SetConstants(L);

	return 0;
}

int Shutdown(lua_State *L)
{
	std::vector<CThreadedCurl *> *curlList = NULL;

	if(curlList)
	{
		std::vector<CThreadedCurl *> copy = *curlList;
		std::vector<CThreadedCurl *>::iterator itor = copy.begin();
		while(itor != copy.end())
		{
			delete (*itor);

			itor++;
		}
	}

	g_Curl.erase(L);

	if(g_Curl.empty())
	{
		curl_global_cleanup();
	}
	return 0;
}

#define CurlOptionToLua(name, type, oldValue)								\
	if(CURLOPTTYPE_ ## type == CURLOPTTYPE_LONG)							\
	{																		\
		Lua()->SetGlobal( "CURLOPT_" #name , (float) CURLOPT_ ## name );	\
	}

int SetConstants(lua_State *L)
{
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////// M I S C /////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	Lua()->SetGlobal("CURL_HTTP_VERSION_NONE", (float)CURL_HTTP_VERSION_NONE);
	Lua()->SetGlobal("CURL_HTTP_VERSION_1_0", (float)CURL_HTTP_VERSION_1_0);
	Lua()->SetGlobal("CURL_HTTP_VERSION_1_1", (float)CURL_HTTP_VERSION_1_1);

	Lua()->SetGlobal("CURL_SSLVERSION_DEFAULT", (float)CURL_SSLVERSION_DEFAULT);
	Lua()->SetGlobal("CURL_SSLVERSION_TLSv1", (float)CURL_SSLVERSION_TLSv1);
	Lua()->SetGlobal("CURL_SSLVERSION_SSLv2", (float)CURL_SSLVERSION_SSLv2);
	Lua()->SetGlobal("CURL_SSLVERSION_SSLv3", (float)CURL_SSLVERSION_SSLv3);
	
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////// A U T H /////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	Lua()->SetGlobal("CURLAUTH_NONE", (float)CURLAUTH_NONE);
	Lua()->SetGlobal("CURLAUTH_BASIC", (float)CURLAUTH_BASIC);
	Lua()->SetGlobal("CURLAUTH_DIGEST", (float)CURLAUTH_DIGEST);
	Lua()->SetGlobal("CURLAUTH_GSSNEGOTIATE", (float)CURLAUTH_GSSNEGOTIATE);
	Lua()->SetGlobal("CURLAUTH_NTLM", (float)CURLAUTH_NTLM);
	Lua()->SetGlobal("CURLAUTH_DIGEST_IE", (float)CURLAUTH_DIGEST_IE);
	Lua()->SetGlobal("CURLAUTH_ANY", (float)CURLAUTH_ANY);
	Lua()->SetGlobal("CURLAUTH_ANYSAFE", (float)CURLAUTH_ANYSAFE);

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////// O P T I O N S ///////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	/* This is the FILE * or void * the regular output should be written to. */
	CurlOptionToLua(FILE, OBJECTPOINT, 1);

	/* The full URL to get/put */
	CurlOptionToLua(URL,  OBJECTPOINT, 2);

	/* Port number to connect to, if other than default. */
	CurlOptionToLua(PORT, LONG, 3);

	/* Name of proxy to use. */
	CurlOptionToLua(PROXY, OBJECTPOINT, 4);

	/* "name:password" to use when fetching. */
	CurlOptionToLua(USERPWD, OBJECTPOINT, 5);

	/* "name:password" to use with proxy. */
	CurlOptionToLua(PROXYUSERPWD, OBJECTPOINT, 6);

	/* Range to get, specified as an ASCII string. */
	CurlOptionToLua(RANGE, OBJECTPOINT, 7);

	/* not used */

	/* Specified file stream to upload from (use as input): */
	CurlOptionToLua(INFILE, OBJECTPOINT, 9);

	/* Buffer to receive error messages in, must be at least CURL_ERROR_SIZE
	* bytes big. If this is not used, error messages go to stderr instead: */
	CurlOptionToLua(ERRORBUFFER, OBJECTPOINT, 10);

	/* Function that will be called to store the output (instead of fwrite). The
	* parameters will use fwrite() syntax, make sure to follow them. */
	CurlOptionToLua(WRITEFUNCTION, FUNCTIONPOINT, 11);

	/* Function that will be called to read the input (instead of fread). The
	* parameters will use fread() syntax, make sure to follow them. */
	CurlOptionToLua(READFUNCTION, FUNCTIONPOINT, 12);

	/* Time-out the read operation after this amount of seconds */
	CurlOptionToLua(TIMEOUT, LONG, 13);

	/* If the CURLOPT_INFILE is used, this can be used to inform libcurl about
	* how large the file being sent really is. That allows better error
	* checking and better verifies that the upload was successful. -1 means
	* unknown size.
	*
	* For large file support, there is also a _LARGE version of the key
	* which takes an off_t type, allowing platforms with larger off_t
	* sizes to handle larger files.  See below for INFILESIZE_LARGE.
	*/
	CurlOptionToLua(INFILESIZE, LONG, 14);

	/* POST static input fields. */
	CurlOptionToLua(POSTFIELDS, OBJECTPOINT, 15);

	/* Set the referrer page (needed by some CGIs) */
	CurlOptionToLua(REFERER, OBJECTPOINT, 16);

	/* Set the FTP PORT string (interface name, named or numerical IP address)
		Use i.e '-' to use default address. */
	CurlOptionToLua(FTPPORT, OBJECTPOINT, 17);

	/* Set the User-Agent string (examined by some CGIs) */
	CurlOptionToLua(USERAGENT, OBJECTPOINT, 18);

	/* If the download receives less than "low speed limit" bytes/second
	* during "low speed time" seconds, the operations is aborted.
	* You could i.e if you have a pretty high speed connection, abort if
	* it is less than 2000 bytes/sec during 20 seconds.
	*/

	/* Set the "low speed limit" */
	CurlOptionToLua(LOW_SPEED_LIMIT, LONG, 19);

	/* Set the "low speed time" */
	CurlOptionToLua(LOW_SPEED_TIME, LONG, 20);

	/* Set the continuation offset.
	*
	* Note there is also a _LARGE version of this key which uses
	* off_t types, allowing for large file offsets on platforms which
	* use larger-than-32-bit off_t's.  Look below for RESUME_FROM_LARGE.
	*/
	CurlOptionToLua(RESUME_FROM, LONG, 21);

	/* Set cookie in request: */
	CurlOptionToLua(COOKIE, OBJECTPOINT, 22);

	/* This points to a linked list of headers, struct curl_slist kind */
	CurlOptionToLua(HTTPHEADER, OBJECTPOINT, 23);

	/* This points to a linked list of post entries, struct curl_httppost */
	CurlOptionToLua(HTTPPOST, OBJECTPOINT, 24);

	/* name of the file keeping your private SSL-certificate */
	CurlOptionToLua(SSLCERT, OBJECTPOINT, 25);

	/* password for the SSL or SSH private key */
	CurlOptionToLua(KEYPASSWD, OBJECTPOINT, 26);

	/* send TYPE parameter? */
	CurlOptionToLua(CRLF, LONG, 27);

	/* send linked-list of QUOTE commands */
	CurlOptionToLua(QUOTE, OBJECTPOINT, 28);

	/* send FILE * or void * to store headers to, if you use a callback it
		is simply passed to the callback unmodified */
	CurlOptionToLua(WRITEHEADER, OBJECTPOINT, 29);

	/* point to a file to read the initial cookies from, also enables
		"cookie awareness" */
	CurlOptionToLua(COOKIEFILE, OBJECTPOINT, 31);

	/* What version to specifically try to use.
		See CURL_SSLVERSION defines below. */
	CurlOptionToLua(SSLVERSION, LONG, 32);

	/* What kind of HTTP time condition to use, see defines */
	CurlOptionToLua(TIMECONDITION, LONG, 33);

	/* Time to use with the above condition. Specified in number of seconds
		since 1 Jan 1970 */
	CurlOptionToLua(TIMEVALUE, LONG, 34);

	/* 35 = OBSOLETE */

	/* Custom request, for customizing the get command like
		HTTP: DELETE, TRACE and others
		FTP: to use a different list command
		*/
	CurlOptionToLua(CUSTOMREQUEST, OBJECTPOINT, 36);

	/* HTTP request, for odd commands like DELETE, TRACE and others */
	CurlOptionToLua(STDERR, OBJECTPOINT, 37);

	/* 38 is not used */

	/* send linked-list of post-transfer QUOTE commands */
	CurlOptionToLua(POSTQUOTE, OBJECTPOINT, 39);

	/* Pass a pointer to string of the output using full variable-replacement
		as described elsewhere. */
	CurlOptionToLua(WRITEINFO, OBJECTPOINT, 40);

	CurlOptionToLua(VERBOSE, LONG, 41);      /* talk a lot */
	CurlOptionToLua(HEADER, LONG, 42);       /* throw the header out too */
	CurlOptionToLua(NOPROGRESS, LONG, 43);   /* shut off the progress meter */
	CurlOptionToLua(NOBODY, LONG, 44);       /* use HEAD to get http document */
	CurlOptionToLua(FAILONERROR, LONG, 45);  /* no output on http error codes >= 300 */
	CurlOptionToLua(UPLOAD, LONG, 46);       /* this is an upload */
	CurlOptionToLua(POST, LONG, 47);         /* HTTP POST method */
	CurlOptionToLua(DIRLISTONLY, LONG, 48);  /* return bare names when listing directories */

	CurlOptionToLua(APPEND, LONG, 50);       /* Append instead of overwrite on upload! */

	/* Specify whether to read the user+password from the .netrc or the URL.
	* This must be one of the CURL_NETRC_* enums below. */
	CurlOptionToLua(NETRC, LONG, 51);

	CurlOptionToLua(FOLLOWLOCATION, LONG, 52);  /* use Location: Luke! */

	CurlOptionToLua(TRANSFERTEXT, LONG, 53); /* transfer data in text/ASCII format */
	CurlOptionToLua(PUT, LONG, 54);          /* HTTP PUT */

	/* 55 = OBSOLETE */

	/* Function that will be called instead of the internal progress display
	* function. This function should be defined as the curl_progress_callback
	* prototype defines. */
	CurlOptionToLua(PROGRESSFUNCTION, FUNCTIONPOINT, 56);

	/* Data passed to the progress callback */
	CurlOptionToLua(PROGRESSDATA, OBJECTPOINT, 57);

	/* We want the referrer field set automatically when following locations */
	CurlOptionToLua(AUTOREFERER, LONG, 58);

	/* Port of the proxy, can be set in the proxy string as well with:
		"[host]:[port]" */
	CurlOptionToLua(PROXYPORT, LONG, 59);

	/* size of the POST input data, if strlen() is not good to use */
	CurlOptionToLua(POSTFIELDSIZE, LONG, 60);

	/* tunnel non-http operations through a HTTP proxy */
	CurlOptionToLua(HTTPPROXYTUNNEL, LONG, 61);

	/* Set the interface string to use as outgoing network interface */
	CurlOptionToLua(INTERFACE, OBJECTPOINT, 62);

	/* Set the krb4/5 security level, this also enables krb4/5 awareness.  This
	* is a string, 'clear', 'safe', 'confidential' or 'private'.  If the string
	* is set but doesn't match one of these, 'private' will be used.  */
	CurlOptionToLua(KRBLEVEL, OBJECTPOINT, 63);

	/* Set if we should verify the peer in ssl handshake, set 1 to verify. */
	CurlOptionToLua(SSL_VERIFYPEER, LONG, 64);

	/* The CApath or CAfile used to validate the peer certificate
		this option is used only if SSL_VERIFYPEER is true */
	CurlOptionToLua(CAINFO, OBJECTPOINT, 65);

	/* 66 = OBSOLETE */
	/* 67 = OBSOLETE */

	/* Maximum number of http redirects to follow */
	CurlOptionToLua(MAXREDIRS, LONG, 68);

	/* Pass a long set to 1 to get the date of the requested document (if
		possible)! Pass a zero to shut it off. */
	CurlOptionToLua(FILETIME, LONG, 69);

	/* This points to a linked list of telnet options */
	CurlOptionToLua(TELNETOPTIONS, OBJECTPOINT, 70);

	/* Max amount of cached alive connections */
	CurlOptionToLua(MAXCONNECTS, LONG, 71);

	/* What policy to use when closing connections when the cache is filled
		up */
	CurlOptionToLua(CLOSEPOLICY, LONG, 72);

	/* 73 = OBSOLETE */

	/* Set to explicitly use a new connection for the upcoming transfer.
		Do not use this unless you're absolutely sure of this, as it makes the
		operation slower and is less friendly for the network. */
	CurlOptionToLua(FRESH_CONNECT, LONG, 74);

	/* Set to explicitly forbid the upcoming transfer's connection to be re-used
		when done. Do not use this unless you're absolutely sure of this, as it
		makes the operation slower and is less friendly for the network. */
	CurlOptionToLua(FORBID_REUSE, LONG, 75);

	/* Set to a file name that contains random data for libcurl to use to
		seed the random engine when doing SSL connects. */
	CurlOptionToLua(RANDOM_FILE, OBJECTPOINT, 76);

	/* Set to the Entropy Gathering Daemon socket pathname */
	CurlOptionToLua(EGDSOCKET, OBJECTPOINT, 77);

	/* Time-out connect operations after this amount of seconds, if connects
		are OK within this time, then fine... This only aborts the connect
		phase. [Only works on unix-style/SIGALRM operating systems] */
	CurlOptionToLua(CONNECTTIMEOUT, LONG, 78);

	/* Function that will be called to store headers (instead of fwrite). The
	* parameters will use fwrite() syntax, make sure to follow them. */
	CurlOptionToLua(HEADERFUNCTION, FUNCTIONPOINT, 79);

	/* Set this to force the HTTP request to get back to GET. Only really usable
		if POST, PUT or a custom request have been used first.
	*/
	CurlOptionToLua(HTTPGET, LONG, 80);

	/* Set if we should verify the Common name from the peer certificate in ssl
	* handshake, set 1 to check existence, 2 to ensure that it matches the
	* provided hostname. */
	CurlOptionToLua(SSL_VERIFYHOST, LONG, 81);

	/* Specify which file name to write all known cookies in after completed
		operation. Set file name to "-" (dash) to make it go to stdout. */
	CurlOptionToLua(COOKIEJAR, OBJECTPOINT, 82);

	/* Specify which SSL ciphers to use */
	CurlOptionToLua(SSL_CIPHER_LIST, OBJECTPOINT, 83);

	/* Specify which HTTP version to use! This must be set to one of the
		CURL_HTTP_VERSION* enums set below. */
	CurlOptionToLua(HTTP_VERSION, LONG, 84);

	/* Specifically switch on or off the FTP engine's use of the EPSV command. By
		default, that one will always be attempted before the more traditional
		PASV command. */
	CurlOptionToLua(FTP_USE_EPSV, LONG, 85);

	/* type of the file keeping your SSL-certificate ("DER", "PEM", "ENG") */
	CurlOptionToLua(SSLCERTTYPE, OBJECTPOINT, 86);

	/* name of the file keeping your private SSL-key */
	CurlOptionToLua(SSLKEY, OBJECTPOINT, 87);

	/* type of the file keeping your private SSL-key ("DER", "PEM", "ENG") */
	CurlOptionToLua(SSLKEYTYPE, OBJECTPOINT, 88);

	/* crypto engine for the SSL-sub system */
	CurlOptionToLua(SSLENGINE, OBJECTPOINT, 89);

	/* set the crypto engine for the SSL-sub system as default
		the param has no meaning...
	*/
	CurlOptionToLua(SSLENGINE_DEFAULT, LONG, 90);

	/* Non-zero value means to use the global dns cache */
	CurlOptionToLua(DNS_USE_GLOBAL_CACHE, LONG, 91); /* To become OBSOLETE soon */

	/* DNS cache timeout */
	CurlOptionToLua(DNS_CACHE_TIMEOUT, LONG, 92);

	/* send linked-list of pre-transfer QUOTE commands */
	CurlOptionToLua(PREQUOTE, OBJECTPOINT, 93);

	/* set the debug function */
	CurlOptionToLua(DEBUGFUNCTION, FUNCTIONPOINT, 94);

	/* set the data for the debug function */
	CurlOptionToLua(DEBUGDATA, OBJECTPOINT, 95);

	/* mark this as start of a cookie session */
	CurlOptionToLua(COOKIESESSION, LONG, 96);

	/* The CApath directory used to validate the peer certificate
		this option is used only if SSL_VERIFYPEER is true */
	CurlOptionToLua(CAPATH, OBJECTPOINT, 97);

	/* Instruct libcurl to use a smaller receive buffer */
	CurlOptionToLua(BUFFERSIZE, LONG, 98);

	/* Instruct libcurl to not use any signal/alarm handlers, even when using
		timeouts. This option is useful for multi-threaded applications.
		See libcurl-the-guide for more background information. */
	CurlOptionToLua(NOSIGNAL, LONG, 99);

	/* Provide a CURLShare for mutexing non-ts data */
	CurlOptionToLua(SHARE, OBJECTPOINT, 100);

	/* indicates type of proxy. accepted values are CURLPROXY_HTTP (default);
		CURLPROXY_SOCKS4, CURLPROXY_SOCKS4A and CURLPROXY_SOCKS5. */
	CurlOptionToLua(PROXYTYPE, LONG, 101);

	/* Set the Accept-Encoding string. Use this to tell a server you would like
		the response to be compressed. */
	CurlOptionToLua(ENCODING, OBJECTPOINT, 102);

	/* Set pointer to private data */
	CurlOptionToLua(PRIVATE, OBJECTPOINT, 103);

	/* Set aliases for HTTP 200 in the HTTP Response header */
	CurlOptionToLua(HTTP200ALIASES, OBJECTPOINT, 104);

	/* Continue to send authentication (user+password) when following locations,
		even when hostname changed. This can potentially send off the name
		and password to whatever host the server decides. */
	CurlOptionToLua(UNRESTRICTED_AUTH, LONG, 105);

	/* Specifically switch on or off the FTP engine's use of the EPRT command ( it
		also disables the LPRT attempt). By default, those ones will always be
		attempted before the good old traditional PORT command. */
	CurlOptionToLua(FTP_USE_EPRT, LONG, 106);

	/* Set this to a bitmask value to enable the particular authentications
		methods you like. Use this in combination with CURLOPT_USERPWD.
		Note that setting multiple bits may cause extra network round-trips. */
	CurlOptionToLua(HTTPAUTH, LONG, 107);

	/* Set the ssl context callback function, currently only for OpenSSL ssl_ctx
		in second argument. The function must be matching the
		curl_ssl_ctx_callback proto. */
	CurlOptionToLua(SSL_CTX_FUNCTION, FUNCTIONPOINT, 108);

	/* Set the userdata for the ssl context callback function's third
		argument */
	CurlOptionToLua(SSL_CTX_DATA, OBJECTPOINT, 109);

	/* FTP Option that causes missing dirs to be created on the remote server.
		In 7.19.4 we introduced the convenience enums for this option using the
		CURLFTP_CREATE_DIR prefix.
	*/
	CurlOptionToLua(FTP_CREATE_MISSING_DIRS, LONG, 110);

	/* Set this to a bitmask value to enable the particular authentications
		methods you like. Use this in combination with CURLOPT_PROXYUSERPWD.
		Note that setting multiple bits may cause extra network round-trips. */
	CurlOptionToLua(PROXYAUTH, LONG, 111);

	/* FTP option that changes the timeout, in seconds, associated with
		getting a response.  This is different from transfer timeout time and
		essentially places a demand on the FTP server to acknowledge commands
		in a timely manner. */
	CurlOptionToLua(FTP_RESPONSE_TIMEOUT, LONG, 112);

	/* Set this option to one of the CURL_IPRESOLVE_* defines (see below) to
		tell libcurl to resolve names to those IP versions only. This only has
		affect on systems with support for more than one, i.e IPv4 _and_ IPv6. */
	CurlOptionToLua(IPRESOLVE, LONG, 113);

	/* Set this option to limit the size of a file that will be downloaded from
		an HTTP or FTP server.

		Note there is also _LARGE version which adds large file support for
		platforms which have larger off_t sizes.  See MAXFILESIZE_LARGE below. */
	CurlOptionToLua(MAXFILESIZE, LONG, 114);

	/* See the comment for INFILESIZE above, but in short, specifies
	* the size of the file being uploaded.  -1 means unknown.
	*/
	CurlOptionToLua(INFILESIZE_LARGE, OFF_T, 115);

	/* Sets the continuation offset.  There is also a LONG version of this;
	* look above for RESUME_FROM.
	*/
	CurlOptionToLua(RESUME_FROM_LARGE, OFF_T, 116);

	/* Sets the maximum size of data that will be downloaded from
	* an HTTP or FTP server.  See MAXFILESIZE above for the LONG version.
	*/
	CurlOptionToLua(MAXFILESIZE_LARGE, OFF_T, 117);

	/* Set this option to the file name of your .netrc file you want libcurl
		to parse (using the CURLOPT_NETRC option). If not set, libcurl will do
		a poor attempt to find the user's home directory and check for a .netrc
		file in there. */
	CurlOptionToLua(NETRC_FILE, OBJECTPOINT, 118);

	/* Enable SSL/TLS for FTP, pick one of:
		CURLFTPSSL_TRY     - try using SSL, proceed anyway otherwise
		CURLFTPSSL_CONTROL - SSL for the control connection or fail
		CURLFTPSSL_ALL     - SSL for all communication or fail
	*/
	CurlOptionToLua(USE_SSL, LONG, 119);

	/* The _LARGE version of the standard POSTFIELDSIZE option */
	CurlOptionToLua(POSTFIELDSIZE_LARGE, OFF_T, 120);

	/* Enable/disable the TCP Nagle algorithm */
	CurlOptionToLua(TCP_NODELAY, LONG, 121);

	/* 122 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
	/* 123 OBSOLETE. Gone in 7.16.0 */
	/* 124 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
	/* 125 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
	/* 126 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
	/* 127 OBSOLETE. Gone in 7.16.0 */
	/* 128 OBSOLETE. Gone in 7.16.0 */

	/* When FTP over SSL/TLS is selected (with CURLOPT_USE_SSL); this option
		can be used to change libcurl's default action which is to first try
		"AUTH SSL" and then "AUTH TLS" in this order, and proceed when a OK
		response has been received.

		Available parameters are:
		CURLFTPAUTH_DEFAULT - let libcurl decide
		CURLFTPAUTH_SSL     - try "AUTH SSL" first, then TLS
		CURLFTPAUTH_TLS     - try "AUTH TLS" first, then SSL
	*/
	CurlOptionToLua(FTPSSLAUTH, LONG, 129);

	CurlOptionToLua(IOCTLFUNCTION, FUNCTIONPOINT, 130);
	CurlOptionToLua(IOCTLDATA, OBJECTPOINT, 131);

	/* 132 OBSOLETE. Gone in 7.16.0 */
	/* 133 OBSOLETE. Gone in 7.16.0 */

	/* zero terminated string for pass on to the FTP server when asked for
		"account" info */
	CurlOptionToLua(FTP_ACCOUNT, OBJECTPOINT, 134);

	/* feed cookies into cookie engine */
	CurlOptionToLua(COOKIELIST, OBJECTPOINT, 135);

	/* ignore Content-Length */
	CurlOptionToLua(IGNORE_CONTENT_LENGTH, LONG, 136);

	/* Set to non-zero to skip the IP address received in a 227 PASV FTP server
		response. Typically used for FTP-SSL purposes but is not restricted to
		that. libcurl will then instead use the same IP address it used for the
		control connection. */
	CurlOptionToLua(FTP_SKIP_PASV_IP, LONG, 137);

	/* Select "file method" to use when doing FTP, see the curl_ftpmethod
		above. */
	CurlOptionToLua(FTP_FILEMETHOD, LONG, 138);

	/* Local port number to bind the socket to */
	CurlOptionToLua(LOCALPORT, LONG, 139);

	/* Number of ports to try, including the first one set with LOCALPORT.
		Thus, setting it to 1 will make no additional attempts but the first.
	*/
	CurlOptionToLua(LOCALPORTRANGE, LONG, 140);

	/* no transfer, set up connection and let application use the socket by
		extracting it with CURLINFO_LASTSOCKET */
	CurlOptionToLua(CONNECT_ONLY, LONG, 141);

	/* Function that will be called to convert from the
		network encoding (instead of using the iconv calls in libcurl) */
	CurlOptionToLua(CONV_FROM_NETWORK_FUNCTION, FUNCTIONPOINT, 142);

	/* Function that will be called to convert to the
		network encoding (instead of using the iconv calls in libcurl) */
	CurlOptionToLua(CONV_TO_NETWORK_FUNCTION, FUNCTIONPOINT, 143);

	/* Function that will be called to convert from UTF8
		(instead of using the iconv calls in libcurl)
		Note that this is used only for SSL certificate processing */
	CurlOptionToLua(CONV_FROM_UTF8_FUNCTION, FUNCTIONPOINT, 144);

	/* if the connection proceeds too quickly then need to slow it down */
	/* limit-rate: maximum number of bytes per second to send or receive */
	CurlOptionToLua(MAX_SEND_SPEED_LARGE, OFF_T, 145);
	CurlOptionToLua(MAX_RECV_SPEED_LARGE, OFF_T, 146);

	/* Pointer to command string to send if USER/PASS fails. */
	CurlOptionToLua(FTP_ALTERNATIVE_TO_USER, OBJECTPOINT, 147);

	/* callback function for setting socket options */
	CurlOptionToLua(SOCKOPTFUNCTION, FUNCTIONPOINT, 148);
	CurlOptionToLua(SOCKOPTDATA, OBJECTPOINT, 149);

	/* set to 0 to disable session ID re-use for this transfer, default is
		enabled (== 1) */
	CurlOptionToLua(SSL_SESSIONID_CACHE, LONG, 150);

	/* allowed SSH authentication methods */
	CurlOptionToLua(SSH_AUTH_TYPES, LONG, 151);

	/* Used by scp/sftp to do public/private key authentication */
	CurlOptionToLua(SSH_PUBLIC_KEYFILE, OBJECTPOINT, 152);
	CurlOptionToLua(SSH_PRIVATE_KEYFILE, OBJECTPOINT, 153);

	/* Send CCC (Clear Command Channel) after authentication */
	CurlOptionToLua(FTP_SSL_CCC, LONG, 154);

	/* Same as TIMEOUT and CONNECTTIMEOUT, but with ms resolution */
	CurlOptionToLua(TIMEOUT_MS, LONG, 155);
	CurlOptionToLua(CONNECTTIMEOUT_MS, LONG, 156);

	/* set to zero to disable the libcurl's decoding and thus pass the raw body
		data to the application even when it is encoded/compressed */
	CurlOptionToLua(HTTP_TRANSFER_DECODING, LONG, 157);
	CurlOptionToLua(HTTP_CONTENT_DECODING, LONG, 158);

	/* Permission used when creating new files and directories on the remote
		server for protocols that support it, SFTP/SCP/FILE */
	CurlOptionToLua(NEW_FILE_PERMS, LONG, 159);
	CurlOptionToLua(NEW_DIRECTORY_PERMS, LONG, 160);

	/* Set the behaviour of POST when redirecting. Values must be set to one
		of CURL_REDIR* defines below. This used to be called CURLOPT_POST301 */
	CurlOptionToLua(POSTREDIR, LONG, 161);

	/* used by scp/sftp to verify the host's public key */
	CurlOptionToLua(SSH_HOST_PUBLIC_KEY_MD5, OBJECTPOINT, 162);

	/* Callback function for opening socket (instead of socket(2)). Optionally,
		callback is able change the address or refuse to connect returning
		CURL_SOCKET_BAD.  The callback should have type
		curl_opensocket_callback */
	CurlOptionToLua(OPENSOCKETFUNCTION, FUNCTIONPOINT, 163);
	CurlOptionToLua(OPENSOCKETDATA, OBJECTPOINT, 164);

	/* POST volatile input fields. */
	CurlOptionToLua(COPYPOSTFIELDS, OBJECTPOINT, 165);

	/* set transfer mode (;type=<a|i>) when doing FTP via an HTTP proxy */
	CurlOptionToLua(PROXY_TRANSFER_MODE, LONG, 166);

	/* Callback function for seeking in the input stream */
	CurlOptionToLua(SEEKFUNCTION, FUNCTIONPOINT, 167);
	CurlOptionToLua(SEEKDATA, OBJECTPOINT, 168);

	/* CRL file */
	CurlOptionToLua(CRLFILE, OBJECTPOINT, 169);

	/* Issuer certificate */
	CurlOptionToLua(ISSUERCERT, OBJECTPOINT, 170);

	/* (IPv6) Address scope */
	CurlOptionToLua(ADDRESS_SCOPE, LONG, 171);

	/* Collect certificate chain info and allow it to get retrievable with
		CURLINFO_CERTINFO after the transfer is complete. (Unfortunately) only
		working with OpenSSL-powered builds. */
	CurlOptionToLua(CERTINFO, LONG, 172);

	/* "name" and "pwd" to use when fetching. */
	CurlOptionToLua(USERNAME, OBJECTPOINT, 173);
	CurlOptionToLua(PASSWORD, OBJECTPOINT, 174);

	/* "name" and "pwd" to use with Proxy when fetching. */
	CurlOptionToLua(PROXYUSERNAME, OBJECTPOINT, 175);
	CurlOptionToLua(PROXYPASSWORD, OBJECTPOINT, 176);

	/* Comma separated list of hostnames defining no-proxy zones. These should
		match both hostnames directly, and hostnames within a domain. For
		example, local.com will match local.com and www.local.com, but NOT
		notlocal.com or www.notlocal.com. For compatibility with other
		implementations of this, .local.com will be considered to be the same as
		local.com. A single * is the only valid wildcard, and effectively
		disables the use of proxy. */
	CurlOptionToLua(NOPROXY, OBJECTPOINT, 177);

	/* block size for TFTP transfers */
	CurlOptionToLua(TFTP_BLKSIZE, LONG, 178);

	/* Socks Service */
	CurlOptionToLua(SOCKS5_GSSAPI_SERVICE, OBJECTPOINT, 179);

	/* Socks Service */
	CurlOptionToLua(SOCKS5_GSSAPI_NEC, LONG, 180);

	/* set the bitmask for the protocols that are allowed to be used for the
		transfer, which thus helps the app which takes URLs from users or other
		external inputs and want to restrict what protocol(s) to deal
		with. Defaults to CURLPROTO_ALL. */
	CurlOptionToLua(PROTOCOLS, LONG, 181);

	/* set the bitmask for the protocols that libcurl is allowed to follow to,
		as a subset of the CURLOPT_PROTOCOLS ones. That means the protocol needs
		to be set in both bitmasks to be allowed to get redirected to. Defaults
		to all protocols except FILE and SCP. */
	CurlOptionToLua(REDIR_PROTOCOLS, LONG, 182);

	/* set the SSH knownhost file name to use */
	CurlOptionToLua(SSH_KNOWNHOSTS, OBJECTPOINT, 183);

	/* set the SSH host key callback, must point to a curl_sshkeycallback
		function */
	CurlOptionToLua(SSH_KEYFUNCTION, FUNCTIONPOINT, 184);

	/* set the SSH host key callback custom pointer */
	CurlOptionToLua(SSH_KEYDATA, OBJECTPOINT, 185);

	/* set the SMTP mail originator */
	CurlOptionToLua(MAIL_FROM, OBJECTPOINT, 186);

	/* set the SMTP mail receiver(s) */
	CurlOptionToLua(MAIL_RCPT, OBJECTPOINT, 187);

	/* FTP: send PRET before PASV */
	CurlOptionToLua(FTP_USE_PRET, LONG, 188);

	/* RTSP request method (OPTIONS, SETUP, PLAY, etc...) */
	CurlOptionToLua(RTSP_REQUEST, LONG, 189);

	/* The RTSP session identifier */
	CurlOptionToLua(RTSP_SESSION_ID, OBJECTPOINT, 190);

	/* The RTSP stream URI */
	CurlOptionToLua(RTSP_STREAM_URI, OBJECTPOINT, 191);

	/* The Transport: header to use in RTSP requests */
	CurlOptionToLua(RTSP_TRANSPORT, OBJECTPOINT, 192);

	/* Manually initialize the client RTSP CSeq for this handle */
	CurlOptionToLua(RTSP_CLIENT_CSEQ, LONG, 193);

	/* Manually initialize the server RTSP CSeq for this handle */
	CurlOptionToLua(RTSP_SERVER_CSEQ, LONG, 194);

	/* The stream to pass to INTERLEAVEFUNCTION. */
	CurlOptionToLua(INTERLEAVEDATA, OBJECTPOINT, 195);

	/* Let the application define a custom write method for RTP data */
	CurlOptionToLua(INTERLEAVEFUNCTION, FUNCTIONPOINT, 196);

	/* Turn on wildcard matching */
	CurlOptionToLua(WILDCARDMATCH, LONG, 197);

	/* Directory matching callback called before downloading of an
		individual file (chunk) started */
	CurlOptionToLua(CHUNK_BGN_FUNCTION, FUNCTIONPOINT, 198);

	/* Directory matching callback called after the file (chunk)
		was downloaded, or skipped */
	CurlOptionToLua(CHUNK_END_FUNCTION, FUNCTIONPOINT, 199);

	/* Change match (fnmatch-like) callback for wildcard matching */
	CurlOptionToLua(FNMATCH_FUNCTION, FUNCTIONPOINT, 200);

	/* Let the application define custom chunk data pointer */
	CurlOptionToLua(CHUNK_DATA, OBJECTPOINT, 201);

	/* FNMATCH_FUNCTION user pointer */
	CurlOptionToLua(FNMATCH_DATA, OBJECTPOINT, 202);

	return 0;
}