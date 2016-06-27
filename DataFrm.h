#pragma once
#include "stdafx.h"

typedef struct _SYNCKEY_
{
	int Key;
	int Val;
}SyncKey;

typedef struct _WECHATINFO_
{
	std::string skey;
	std::string wxsid;
	std::string wxuin;
	std::string pass_ticket;
	SyncKey s_synckey[20];
	int SyncCount;
}WeChatInfo;

typedef struct _ChatInfo
{
	std::string UserName;
	std::string NickName;
	std::string HeadImgUrl;
	std::string Signature;
	std::string Province;
	std::string City;
}ChatInfo;


typedef struct _WXMSG_ 
{
	std::string FromUserName;
	std::string ToUserName;
	int Msgtype;
	std::string Content;
	std::string FileName;
	std::string FileSize;
	std::string Url;	//&amp; htmlÖÐ Ìæ»»Îª&


}wxMSG;