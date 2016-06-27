#include "stdafx.h"
#include "httpRequest.h"

int httpResponseSize;

int HttpGetResBufSize()
{
	return httpResponseSize;
}

//��ַ��ȡ����
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
//���ܣ�
//			ģ�����������HTTP������
//������	
//			lpFullAddr	:��ַ	http:// ��ȱʡ		
//			lpMethod	:�ύ��ʽ��GET/POST,ȱʡ��GET
//			lpPostData	:POST���ݣ�GET��ʽȱʡ
//			sPort		:�˿� ȱʡ��80
//���أ�
//			������ҳ�����ı��ͣ�һ��ΪUTF-8���룬�����
//��ע��
//			�ڴ��ļ�����memfile
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char* HttpRequest(char  *lpFullAddr, char * lpMethod, char * lpPostData, int nPostDataLen, short sPort)
{
	int RequestBufSize = 0;	//�������ݵĳ���
	char *lpHostName = GetUrlHost(lpFullAddr);	//��ַȡ����
	char *lpUrl = GetUrlAddr(lpFullAddr);		//��ַȡҳ���ַ 

	HANDLE hmemfile = CreateFileA("memfile",	//�����ڴ��ļ��洢��������
		GENERIC_WRITE | GENERIC_READ,          // д�Ͷ��ļ���
		0,                      // �������д��
		NULL,                   // ȱʡ��ȫ���ԡ�
		CREATE_ALWAYS,          // ����ļ����ڣ�Ҳ������
		FILE_ATTRIBUTE_NORMAL, // һ����ļ���      
		NULL);		 // ģ���ļ�Ϊ�ա�

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

	//����Э��ͷ
	reHeaders = "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n";
	reHeaders += "Referer: http://" + string(lpHostName) + "/\r\n";
	reHeaders += "Accept-Language: zh-cn\r\n";

	//	bRet = HttpAddRequestHeadersA(hRequest,"Content-Type: application/x-www-form-urlencoded",Len(FORMHEADERS),HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
	// 	if(!bRet)			//����cookies
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
	httpResponseSize = RequestBufSize;	//�������ݵĴ�С

	//ȡ�������ش�Э��ͷ ���ж��Ƿ���Ҫ����cookies  //  "Set-Cookie: " 
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

	//д������
	char *lpbuf = (char *)calloc(1, (RequestBufSize + 1)*sizeof(char));	//������һ���ֽڴ��'\0'��Ϊ�ַ�������
	DWORD lpNumofReaded = 0;

	DWORD dwPtr = SetFilePointer(hmemfile, 0, NULL, FILE_BEGIN);	//�Ƶ��ļ���
	if (dwPtr == INVALID_SET_FILE_POINTER)
		DWORD dwError = GetLastError();
	::ReadFile(hmemfile, lpbuf, RequestBufSize, &lpNumofReaded, NULL);
	httpResponseSize = lpNumofReaded;
	CloseHandle(hmemfile);

// 	//����cookie
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
//UTF-8ת��
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

//Base64 ����/����
const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

/* Base64 ���� */
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

/* ת������ */
static char find_pos(char ch)
{
	char *ptr = (char*)strrchr(base, ch);//the last position (the only) in base[] 
	return (ptr - base);
}

/* Base64 ���� */
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

//��ȡʱ�����1970.1.1�����ڵĺ�����
extern char *getTimeStamp()
{
	struct timeb ti;
	ftime(&ti);
	static char TimeStamp[15] = { 0 };
	sprintf_s(TimeStamp, "%lld%u", ti.time, ti.millitm);	//��+����
	return TimeStamp;
}
/////////////////////////////////////
//
//Url ����/����
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

/*  URL���룬��ȡ��PHP 5.2.17
�÷���string UrlDecode(string str_source)
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

/*  URL���룬��ȡ��PHP
�÷���string urlencode(string str_source)
˵������������ -_. ����ȫ�����룬�ո�ᱻ����Ϊ +
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

