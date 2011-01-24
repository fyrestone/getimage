/*!
\file ColorPrint.c
\author LiuBao
\version 1.0
\date 2010/12/28
\brief Windows控制台彩色输出函数实现
*/
#include <stdio.h>		/* vprintf */
#include <stdarg.h>		/* 变参处理 */
#include <Windows.h>	/* 控制台操作 */
#include "ColorPrint.h"

int ColorPrintf(COLOR color, const char *format, ...)
{
	int retVal;
	va_list ap;
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);

	(void)SetConsoleTextAttribute(hCon, color|0);
	va_start(ap, format);//初始化ap。

	retVal = vprintf(format, ap);

	va_end(ap);//清理ap。
	(void)SetConsoleTextAttribute(hCon, SILVER|0);

	return retVal;
}

void PrintAllColor()
{
	int i;
	const char *colorName[] = 
	{
		"BLACK", "NAVY", "GREEN", "TEAL",
		"MAROON", "PURPLE", "OLIVE", "SILVER", "GRAY",
		"BLUE", "LIME", "AQUA", "RED", "FUCHSIA",
		"YELLOW", "WHITE"
	};

	for(i = 0 ; i < sizeof(colorName) / sizeof(const char *) ; ++i)
		ColorPrintf((COLOR)i, "%s\n", colorName[i]);
}