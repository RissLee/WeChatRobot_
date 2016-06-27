// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  �� Windows ͷ�ļ����ų�����ʹ�õ���Ϣ
// Windows ͷ�ļ�: 
#include <windows.h>

// C ����ʱͷ�ļ�
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
//Duilib
#include <ObjBase.h>
#include "UIlib.h"

using namespace DuiLib;


#ifdef _UNICODE
#	ifdef _DEBUG
#		pragma comment(lib, "DuiLib_ud.lib")
#	else
#		pragma comment(lib, "DuiLib_u.lib")
#	endif
#else
#	ifdef _DEBUG
#		pragma comment(lib, "DuiLib_d.lib")
#	else
#		pragma comment(lib, "DuiLib.lib")
#	endif
#endif

#include "json.h"

#ifdef _DEBUG
	#pragma comment(lib,"json_vc71_libmtd.lib")
#else
	#pragma comment(lib,"json_vc71_libmt.lib")
#endif // DEBUG
