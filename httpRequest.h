#include <string>
#include <windows.h>
#include "Wininet.h"
#include <sys/timeb.h>

#pragma comment(lib,"Wininet.lib")
using namespace std;


extern int HttpGetResBufSize();
extern char* HttpRequest(char  *lpFullAddr, char * lpMethod = "GET", char * lpPostData = NULL, int nPostDataLen = 0, short sPort = 80);

//转码
extern string UTF8ToANSI(string& str);
extern string ANSIToUTF8(string &str);
extern string UnicodeToANSI(const wstring& str);
extern wstring ANSIToUnicode(const string& str);
extern string UnicodeToUTF8(const wstring& str);
extern wstring UTF8ToUnicode(const string& str);


extern char *base64_encode(const char* data, int data_len);
extern char *base64_decode(const char *data, int data_len);

//获取时间戳，1970.1.1到现在的毫秒数
extern char *getTimeStamp();

extern string UrlDecode(string &str_source);
extern string UrlEncode(string &str_source);