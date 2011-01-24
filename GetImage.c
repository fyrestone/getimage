/*!
\file GetImage.c
\author LiuBao
\version 1.0
\date 2011/1/23
\brief GetImage的main函数及针对不同参数的处理函数
*/
#include <stdio.h>
#include <conio.h>					/* 使用_getch */
#include "ProjDef.h"
#include "ArgParser\carg_parser.h"	/* 使用ArgParser参数解析 */
#include "ColorPrint.h"
#include "Process.h"

#define PROGRAM_NAME "GETIMAGE"	///< 程序名
#define PROGRAM_YEAR "2010"		///< 年份
#define PROGRAM_VERSION "1.5"	///< 版本

/// 按任意键退出
#define PRESS_ANY_KEY_AND_CONTINUE			\
	do										\
	{										\
		ColorPrintf(WHITE, ".");			\
		_getch();							\
	}while(0)

/*!
测试Debug宏
*/
void TestDebugFlag()
{
	ColorPrintf(AQUA, "开始测试debug宏：\n");

#ifdef _DEBUG
	puts("当前宏为：_DEBUG");
#endif
#ifdef NDEBUG
	puts("当前宏为：NDEBUG");
#endif
}

/*!
打印版权头
*/
void ShowTitle()
{
	printf("\n%s %s\t版权所有(c) 2009-2010 刘宝 %s\n\n", PROGRAM_NAME, PROGRAM_VERSION, __DATE__);
}

/*!
打印帮助信息
*/
void ShowHelp()
{
	printf("用法：\tgetimage [-f|-d] <文件或设备>\n");
	printf("选项：\n");
	printf("\t-h, --help\t\t显示本帮助\n");
	printf("\t-v, --version\t\t显示本程序名及版本号\n");
	printf("\t-f, --file\t\t打开一个文件\n");
	printf("\t-d, --device\t\t打开一个设备\n");
	printf("默认：\n当无选项时将输入的第一个参数作为文件处理：\n"
		"若为ISO，则自动提取其中的启动磁盘映像到ISO文件所在目录，生成的IMG与ISO同名；\n"
		"若为IMG，则显示其规格信息\n");
}

/*!
打印错误信息
*/
void ShowError(const char *msg, const char *selfPath)
{
	if(msg && msg[0])
	{
		ColorPrintf(RED, "%s：%s\n", PROGRAM_NAME, msg);
		ColorPrintf(WHITE, "请尝试 “%s --help” 获得帮助信息！\n", selfPath);
	}
}

/*!
默认选项处理函数
*/
int DefaultOption(int argc, char **argv)
{
	int retVal = FAILED;

	media_t media;

	if(argc != 2 || !(media = OpenMedia(argv[1], 128000)))
		ColorPrintf(RED, "输入文件：%s 映射失败！\n", argv[1]);
	else
	{
		ColorPrintf(WHITE, "%s", "检测到输入文件为");

		switch(GetInputType(media))
		{
		case ISO:
			ColorPrintf(LIME, "%s", "ISO");
			ColorPrintf(WHITE, "，默认作为Acronis启动ISO处理：\n\n");
			DisplayISOInfo(media);
			retVal = DumpIMGFromISO(media, argv[1]);
			break;
		case IMG:
			ColorPrintf(LIME, "%s", "IMG");
			ColorPrintf(WHITE, "，默认显示IMG磁盘映像规格：\n\n");
			retVal = DisplayIMGInfo(media);
			break;
		default:
			ColorPrintf(RED, "%s\n", "未知类型");
		}

		CloseMedia(&media);
	}

	return retVal;
}

int main(int argc, char **argv)
{
	int argIndex;
	struct Arg_parser parser;
	const struct ap_Option options[] =
	{
		/*
		短选项超过unsigned char范围则认为无短选项，
		长选项为NULL则认为无长选项，
		ap_yes代表（如：-a）选项后面跟有参数，
		ap_no代表无参数，ap_maybe代表参数可有可无
		*/

		/* 短选项，	对应长参数，	参数 */
		{'f',		"file",		ap_yes},
		{'d',		"device",	ap_yes},
		{'h',		"help",		ap_no},
		{'v',		"version",	ap_no},
		{0,			0,			ap_no}
	};

	ShowTitle();//打印版权头

	/* 初始化参数解析 */
	if(!ap_init(&parser, argc, (const char * const *)argv, options, 0))
	{
		ColorPrintf(RED, "内存耗尽！\n");
		PRESS_ANY_KEY_AND_CONTINUE;
		return -1;
	}

	/* 如果参数错误 */
	if(ap_error(&parser))
	{
		ShowError(ap_error(&parser), argv[0]);
		PRESS_ANY_KEY_AND_CONTINUE;
		return -1;
	}

	/* 如果解析出的参数个数为0 */
	if(!ap_arguments(&parser))
		ShowHelp();

	/* 遍历解析出的参数 */
	for(argIndex = 0; argIndex < ap_arguments(&parser); ++argIndex)
	{
		const int code = ap_code(&parser, argIndex);//解析出的短选项
		const char *arg = ap_argument(&parser, argIndex);//解析出的选项对应的参数

		switch(code)
		{
		case 'f':
			printf("检测到参数为：%s\n", arg);
			break;
		case 'd':
			printf("检测到参数为：%s\n", arg);
			break;
		case 'h':
			ShowHelp();
			break;
		case 'v'://啥都不做
			break;
		default:
			if(!argIndex)
				DefaultOption(argc, argv);
		}

		if(!code) break;
	}

	PRESS_ANY_KEY_AND_CONTINUE;
	return 0;
}

