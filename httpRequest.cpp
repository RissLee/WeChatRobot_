#include "stdafx.h"
#include "httpRequest.h"

int httpResponseSize;

int HttpGetResBufSize()
{
	return httpResponseSize;
}

//网址提取域名
char *GetUrlHost(char *fulladdr)
{
	static char HostName[1024] = { 0 };
	char *pos = strstr(fulladdr, "http://");
	if (pos != NULL)
		strcpy_s(HostName, pos + 7);
	else
	{
		pos = strstr(fulladdr, "https://");
		if (pos != NULL)
			strcpy_s(HostName, pos + 8);
		else
			strcpy_s(HostName, fulladdr);
	}
	string hostAddr(HostName);
	size_t endPos = hostAddr.find('/');
	if (endPos != string::npos)
		HostName[endPos] = '\0';
	return HostName;

}
char *GetUrlAddr(char *fulladdr)
{
	char *UrlTemp;
	char *pos = strstr(fulladdr, "http://");
	if (pos != NULL)
		UrlTemp = pos + 7;
	else
	{
		pos = strstr(fulladdr, "https://");
		if (NULL != pos)
			UrlTemp = pos + 8;
		else
			UrlTemp = fulladdr;
	}

	char *posStar = strstr(UrlTemp, "/");
	return posStar;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//功能：
//			模拟浏览器发送HTTP请求函数
//参数：	
//			lpFullAddr	:网址	http:// 可缺省		
//			lpMethod	:提交方式：GET/POST,缺省：GET
//			lpPostData	:POST数据，GET方式缺省
//			sPort		:端口 缺省：80
//返回：
//			返回网页数据文本型，一般为UTF-8编码，需解码
//备注：
//			内存文件名：memfile
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char* HttpRequest(char  *lpFullAddr, char * lpMethod, char * lpPostData, int nPostDataLen, short sPort)
{
	int RequestBufSize = 0;	//返回数据的长度
	char *lpHostName = GetUrlHost(lpFullAddr);	//网址取域名
	char *lpUrl = GetUrlAddr(lpFullAddr);		//网址取页面地址 

	HANDLE hmemfile = CreateFileA("memfile",	//创建内存文件存储返回数据
		GENERIC_WRITE | GENERIC_READ,          // 写和读文件。
		0,                      // 不共享读写。
		NULL,                   // 缺省安全属性。
		CREATE_ALWAYS,          // 如果文件存在，也创建。
		FILE_ATTRIBUTE_NORMAL, // 一般的文件。      
		NULL);		 // 模板文件为空。

	if (!nPostDataLen && lpPostData)
		nPostDataLen = strlen(lpPostData);

	HINTERNET hInternet = nullptr, hConnect = nullptr, hRequest = nullptr;

	BOOL bRet;

	std::string strResponse, reHeaders;
	char* ChromeName = "Mozilla/5.0 (Windows NT 10.0; WOW64; rv:46.0) Gecko/20100101 Firefox/46.0";
	hInternet = (HINSTANCE)InternetOpenA(ChromeName, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet)
		goto Ret0;

	hConnect = (HINSTANCE)InternetConnectA(hInternet, lpHostName, sPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect)
		goto Ret0;

	hRequest = (HINSTANCE)HttpOpenRequestA(hConnect, lpMethod, lpUrl, "HTTP/1.1", NULL, NULL, INTERNET_FLAG_RELOAD, 0);
	if (!hRequest)
		goto Ret0;

	//补充协议头
	reHeaders = "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n";
	reHeaders += "Referer: http://" + string(lpHostName) + "/\r\n";
	reHeaders += "Accept-Language: zh-cn\r\n";

	//	bRet = HttpAddRequestHeadersA(hRequest,"Content-Type: application/x-www-form-urlencoded",Len(FORMHEADERS),HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
	// 	if(!bRet)			//发送cookies
	// 	goto Ret0;
	if (lpMethod == "Post")
	{
		reHeaders += "Content-Type: application/x-www-form-urlencoded\r\n";
		reHeaders += "Content-Length: " + nPostDataLen;
		reHeaders += "\r\n";
	}

	bRet = HttpSendRequestA(hRequest, reHeaders.c_str(), reHeaders.size(), lpPostData, nPostDataLen);
	//////

	while (TRUE)
	{
		char cReadBuffer[4096];
		unsigned long lNumberOfBytesRead;
		bRet = InternetReadFile(hRequest, cReadBuffer, sizeof(cReadBuffer)-1, &lNumberOfBytesRead);

		DWORD dwWritenSize = 0;
		WriteFile(hmemfile, cReadBuffer, lNumberOfBytesRead, &dwWritenSize, NULL);

		RequestBufSize += lNumberOfBytesRead;

		if (!bRet || !lNumberOfBytesRead)
			break;
		cReadBuffer[lNumberOfBytesRead] = 0;
		strResponse = strResponse + cReadBuffer;
	}
	httpResponseSize = RequestBufSize;	//返回数据的大小

	//取服务器回传协议头 来判断是否需要设置cookies  //  "Set-Cookie: " 
	char szRequest[10240] = { 0 };
	DWORD szRequestSize = 1024;
	BOOL res = HttpQueryInfoA(hRequest, 22, szRequest, &szRequestSize, 0);
	if (!res)
	{
		DWORD err = GetLastError();
	}
Ret0:
	if (hRequest)
		InternetCloseHandle(hRequest);
	if (hConnect)
		InternetCloseHandle(hConnect);
	if (hInternet)
		InternetCloseHandle(hInternet);

	// 	if (lpHostName)
	// 		free(lpHostName);
	// 	if (lpUrl)
	// 		free(lpUrl);
	//	return strResponse;

	//写出数据
	char *lpbuf = (char *)calloc(1, (RequestBufSize + 1)*sizeof(char));	//多申请一个字节存放'\0'作为字符串结束
	DWORD lpNumofReaded = 0;

	DWORD dwPtr = SetFilePointer(hmemfile, 0, NULL, FILE_BEGIN);	//移到文件首
	if (dwPtr == INVALID_SET_FILE_POINTER)
		DWORD dwError = GetLastError();
	::ReadFile(hmemfile, lpbuf, RequestBufSize, &lpNumofReaded, NULL);
	httpResponseSize = lpNumofReaded;
	CloseHandle(hmemfile);

// 	//设置cookie
// 	string urlHost = string("http://") + lpHostName;
// 	char *nextToken = NULL;
// 	char *strcookie = strtok_s(szRequest, "\r\n", &nextToken);
// 	while (strcookie)
// 	{
// 		char *pos = strstr(strcookie, "Set-Cookie: ");
// 		if (pos)
// 		{
// 			pos += 12;
// 			InternetSetCookieA(urlHost.c_str(), NULL, pos);
// 		}
// 
// 		strcookie = strtok_s(NULL, "\r\n", &nextToken);
// 	}

	return lpbuf;
}
//UTF-8转换
wstring ANSIToUnicode(const string& str)
{
	int len = 0;
	len = str.length();
	int unicodeLen = ::MultiByteToWideChar(CP_ACP,
		0,
		str.c_str(),
		-1,
		NULL,
		0);
	wchar_t * pUnicode;
	pUnicode = new wchar_t[unicodeLen + 1];
	memset(pUnicode, 0, (unicodeLen + 1)*sizeof(wchar_t));
	::MultiByteToWideChar(CP_ACP,
		0,
		str.c_str(),
		-1,
		(LPWSTR)pUnicode,
		unicodeLen);
	wstring rt;
	rt = (wchar_t*)pUnicode;
	delete pUnicode;
	return rt;
}
string UnicodeToANSI(const wstring& str)
{
	char*     pElementText;
	int    iTextLen;
	// wide char to multi char
	iTextLen = WideCharToMultiByte(CP_ACP,
		0,
		str.c_str(),
		-1,
		NULL,
		0,
		NULL,
		NULL);
	pElementText = new char[iTextLen + 1];
	memset((void*)pElementText, 0, sizeof(char)* (iTextLen + 1));
	::WideCharToMultiByte(CP_ACP,
		0,
		str.c_str(),
		-1,
		pElementText,
		iTextLen,
		NULL,
		NULL);
	string strText;
	strText = pElementText;
	delete[] pElementText;
	return strText;
}
wstring UTF8ToUnicode(const string& str)
{
	int len = 0;
	len = str.length();
	int unicodeLen = ::MultiByteToWideChar(CP_UTF8,
		0,
		str.c_str(),
		-1,
		NULL,
		0);
	wchar_t * pUnicode;
	pUnicode = new wchar_t[unicodeLen + 1];
	memset(pUnicode, 0, (unicodeLen + 1)*sizeof(wchar_t));
	::MultiByteToWideChar(CP_UTF8,
		0,
		str.c_str(),
		-1,
		(LPWSTR)pUnicode,
		unicodeLen);
	wstring rt;
	rt = (wchar_t*)pUnicode;
	delete pUnicode;
	return rt;
}
string UnicodeToUTF8(const wstring& str)
{
	char*     pElementText;
	int    iTextLen;
	// wide char to multi char
	iTextLen = WideCharToMultiByte(CP_UTF8,
		0,
		str.c_str(),
		-1,
		NULL,
		0,
		NULL,
		NULL);
	pElementText = new char[iTextLen + 1];
	memset((void*)pElementText, 0, sizeof(char)* (iTextLen + 1));
	::WideCharToMultiByte(CP_UTF8,
		0,
		str.c_str(),
		-1,
		pElementText,
		iTextLen,
		NULL,
		NULL);
	string strText;
	strText = pElementText;
	delete[] pElementText;
	return strText;

}

string UTF8ToANSI(string &str)
{
	return UnicodeToANSI(UTF8ToUnicode(str));
}
string ANSIToUTF8(string &str)
{
	return UnicodeToUTF8(ANSIToUnicode(str));
}

//Base64 编码/解码
const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

/* Base64 编码 */
char *base64_encode(const char* data, int data_len)
{
	//int data_len = strlen(data); 
	int prepare = 0;
	int ret_len;
	int temp = 0;
	char *ret = NULL;
	char *f = NULL;
	int tmp = 0;
	char changed[4];
	int i = 0;
	ret_len = data_len / 3;
	temp = data_len % 3;
	if (temp > 0)
	{
		ret_len += 1;
	}
	ret_len = ret_len * 4 + 1;
	ret = (char *)malloc(ret_len);

	if (ret == NULL)
	{
		printf("No enough memory.\n");
		exit(0);
	}
	memset(ret, 0, ret_len);
	f = ret;
	while (tmp < data_len)
	{
		temp = 0;
		prepare = 0;
		memset(changed, '\0', 4);
		while (temp < 3)
		{
			//printf("tmp = %d\n", tmp); 
			if (tmp >= data_len)
			{
				break;
			}
			prepare = ((prepare << 8) | (data[tmp] & 0xFF));
			tmp++;
			temp++;
		}
		prepare = (prepare << ((3 - temp) * 8));
		//printf("before for : temp = %d, prepare = %d\n", temp, prepare); 
		for (i = 0; i < 4; i++)
		{
			if (temp < i)
			{
				changed[i] = 0x40;
			}
			else
			{
				changed[i] = (prepare >> ((3 - i) * 6)) & 0x3F;
			}
			*f = base[changed[i]];
			//printf("%.2X", changed[i]); 
			f++;
		}
	}
	*f = '\0';

	return ret;

}

/* 转换算子 */
static char find_pos(char ch)
{
	char *ptr = (char*)strrchr(base, ch);//the last position (the only) in base[] 
	return (ptr - base);
}

/* Base64 解码 */
char *base64_decode(const char *data, int data_len)
{
	int ret_len = (data_len / 4) * 3;
	int equal_count = 0;
	char *ret = NULL;
	char *f = NULL;
	int tmp = 0;
	int temp = 0;
	int prepare = 0;
	int i = 0;
	if (*(data + data_len - 1) == '=')
	{
		equal_count += 1;
	}
	if (*(data + data_len - 2) == '=')
	{
		equal_count += 1;
	}
	if (*(data + data_len - 3) == '=')
	{//seems impossible 
		equal_count += 1;
	}
	switch (equal_count)
	{
	case 0:
		ret_len += 4;//3 + 1 [1 for NULL] 
		break;
	case 1:
		ret_len += 4;//Ceil((6*3)/8)+1 
		break;
	case 2:
		ret_len += 3;//Ceil((6*2)/8)+1 
		break;
	case 3:
		ret_len += 2;//Ceil((6*1)/8)+1 
		break;
	}
	ret = (char *)malloc(ret_len);
	if (ret == NULL)
	{
		printf("No enough memory.\n");
		exit(0);
	}
	memset(ret, 0, ret_len);
	f = ret;
	while (tmp < (data_len - equal_count))
	{
		temp = 0;
		prepare = 0;
		while (temp < 4)
		{
			if (tmp >= (data_len - equal_count))
			{
				break;
			}
			prepare = (prepare << 6) | (find_pos(data[tmp]));
			temp++;
			tmp++;
		}
		prepare = prepare << ((4 - temp) * 6);
		for (i = 0; i < 3; i++)
		{
			if (i == temp)
			{
				break;
			}
			*f = (char)((prepare >> ((2 - i) * 8)) & 0xFF);
			f++;
		}
	}
	*f = '\0';
	return ret;
}

//获取时间戳，1970.1.1到现在的毫秒数
extern char *getTimeStamp()
{
	struct timeb ti;
	ftime(&ti);
	static char TimeStamp[15] = { 0 };
	sprintf_s(TimeStamp, "%lld%u", ti.time, ti.millitm);	//秒+毫秒
	return TimeStamp;
}
/////////////////////////////////////
//
//Url 解码/编码
//
///////////////////////////////////
//"hex" to int
static int php_htoi(char *s)
{
	int value;
	int c;

	c = ((unsigned char *)s)[0];
	if (isupper(c))
		c = tolower(c);
	value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

	c = ((unsigned char *)s)[1];
	if (isupper(c))
		c = tolower(c);
	value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

	return (value);
}

/*  URL解码，提取自PHP 5.2.17
用法：string UrlDecode(string str_source)
*/
string UrlDecode(string &str_source)
{
	char const *in_str = str_source.c_str();
	int in_str_len = strlen(in_str);
	int out_str_len = 0;
	string out_str;
	char *str;

	str = _strdup(in_str);
	char *dest = str;
	char *data = str;

	while (in_str_len--) {
		if (*data == '+') {
			*dest = ' ';
		}
		else if (*data == '%' && in_str_len >= 2 && isxdigit((int)*(data + 1))
			&& isxdigit((int)*(data + 2))) {
			*dest = (char)php_htoi(data + 1);
			data += 2;
			in_str_len -= 2;
		}
		else {
			*dest = *data;
		}
		data++;
		dest++;
	}
	*dest = '\0';
	out_str_len = dest - str;
	out_str = str;
	free(str);
	return out_str;
}

/*  URL编码，提取自PHP
用法：string urlencode(string str_source)
说明：仅不编码 -_. 其余全部编码，空格会被编码为 +
*/
string UrlEncode(string &str_source)
{
	char const *in_str = str_source.c_str();
	int in_str_len = strlen(in_str);
	int out_str_len = 0;
	string out_str;
	register unsigned char c;
	unsigned char *to, *start;
	unsigned char const *from, *end;
	unsigned char hexchars[] = "0123456789ABCDEF";

	from = (unsigned char *)in_str;
	end = (unsigned char *)in_str + in_str_len;
	start = to = (unsigned char *)malloc(3 * in_str_len + 1);

	while (from < end) {
		c = *from++;

		if (c == ' ') {
			*to++ = '+';
		}
		else if ((c < '0' && c != '-' && c != '.') ||
			(c < 'A' && c > '9') ||
			(c > 'Z' && c < 'a' && c != '_') ||
			(c > 'z')) {
			to[0] = '%';
			to[1] = hexchars[c >> 4];
			to[2] = hexchars[c & 15];
			to += 3;
		}
		else {
			*to++ = c;
		}
	}
	*to = 0;

	out_str_len = to - start;
	out_str = (char *)start;
	free(start);
	return out_str;
}

