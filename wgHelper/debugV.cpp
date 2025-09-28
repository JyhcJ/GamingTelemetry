
#include "pch.h"
#include <stdio.h>
#include "debugV.h"

void OutputDebugInfo(const char* pszFormat, ...)
{
#ifdef _DEBUG
	// ����̶�ǰ׺�ַ���
	printf(pszFormat);
	const char* prefix = "[������Ϣ] ";
	char szbufFormat[0x1000];
	char szbufFormat_Game[0x1100] = "";
	// ���̶�ǰ׺ƴ�ӵ� szbufFormat_Game
	strcat_s(szbufFormat_Game, prefix);
	va_list argList;
	va_start(argList, pszFormat);//�����б��ʼ��
	vsprintf_s(szbufFormat, pszFormat, argList);
	strcat_s(szbufFormat_Game, szbufFormat);
	OutputDebugStringA(szbufFormat_Game);
	va_end(argList);

#endif
}