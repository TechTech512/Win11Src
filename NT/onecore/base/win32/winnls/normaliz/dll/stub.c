#define _NORMALIZE_
#pragma warning (disable:4716)
#include <windows.h>

WINNORMALIZEAPI
int
WINAPI IdnToAscii(_In_                           DWORD    dwFlags,
                  _In_reads_(cchUnicodeChar) 	 LPCWSTR  lpUnicodeCharStr,
                  _In_                        	 int      cchUnicodeChar,
                  _Out_writes_opt_(cchASCIIChar) LPWSTR   lpASCIICharStr,
                  _In_                        	 int      cchASCIIChar){}
WINNORMALIZEAPI
int
WINAPI IdnToNameprepUnicode(_In_                            	DWORD   dwFlags,
                            _In_reads_(cchUnicodeChar)     	LPCWSTR lpUnicodeCharStr,
                            _In_                            	int     cchUnicodeChar,
                            _Out_writes_opt_(cchNameprepChar)   LPWSTR  lpNameprepCharStr,
                            _In_                            	int     cchNameprepChar){}
WINNORMALIZEAPI
int
WINAPI IdnToUnicode(_In_                         	 DWORD   dwFlags,
                    _In_reads_(cchASCIIChar)    	 LPCWSTR lpASCIICharStr,
                    _In_                         	 int     cchASCIIChar,
                    _Out_writes_opt_(cchUnicodeChar) LPWSTR  lpUnicodeCharStr,
                    _In_                         	 int     cchUnicodeChar){}
WINNORMALIZEAPI
BOOL
WINAPI IsNormalizedString( _In_                   NORM_FORM NormForm,
                           _In_reads_(cwLength)  LPCWSTR   lpString,
                           _In_                   int       cwLength ){}
WINNORMALIZEAPI
int
WINAPI NormalizeString( _In_                          NORM_FORM NormForm,
                        _In_reads_(cwSrcLength)      LPCWSTR   lpSrcString,
                        _In_                          int       cwSrcLength,
                        _Out_writes_opt_(cwDstLength) LPWSTR    lpDstString,
                        _In_                          int       cwDstLength ){}
