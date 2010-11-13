#include <stdio.h>
#include <stdlib.h>
#include <conio.h>					/*!< _getch */
#include "ArgParser\carg_parser.h"
#include "SafeMemory.h"
#include "ColorPrint.h"
#include "Process.h"

#define FAIL -1
#define SUCCESS 0

#define PROGRAM_NAME "GETIMAGE"
#define PROGRAM_YEAR "2010"
#define PROGRAM_VERSION "1.5"

#define PRESS_ANY_KEY_AND_CONTINUE			\
	do										\
	{										\
		ColorPrintf(WHITE, ".");			\
		_getch();							\
	}while(0)

void TestSafeMemory()
{
	ColorPrintf(AQUA, "开始测试SafeMemory模块：\n");
	{
		Malloc IMG_Header;

		if(SafeMalloc(&IMG_Header, 10) != SafeMallocSuccess)
			puts("SafeMallocFailed!");
		else
			puts("SafeMemoryTestSuccess!");

		SafeFree(&IMG_Header);
	}
}

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

void TestColorPrint()
{
	ColorPrintf(AQUA, "开始测试彩色输出：\n");
	PrintAllColor();
}

void ShowTitle()
{
	printf("\n%s %s\t版权所有(c) 2009-2010 刘宝 %s\n\n", PROGRAM_NAME, PROGRAM_VERSION, __DATE__);
}

void ShowHelp()
{
	printf("用法：\tgetimage [选项] 文件或设备\n");
	printf("选项：\n");
	printf("\t-h, --help\t\t显示本帮助\n");
	printf("\t-v, --version\t\t显示本程序名及版本号\n");
	printf("\t-f, --file\t\t打开一个文件\n");
	printf("\t-d, --device\t\t打开一个设备\n");
	printf("特殊：\n\t当无选项时，默认将输入的第一个参数作为文件处理："
		"若为ISO，则自动提取其中的启动磁盘映像到ISO文件所在目录，生成的IMG与ISO同名；"
		"若为IMG，则显示其规格信息；否则显示错误信息。\n");
}

void ShowError(const char *msg, const char *selfPath)
{
	if(msg && msg[0])
	{
		ColorPrintf(RED, "%s：%s\n", PROGRAM_NAME, msg);
		ColorPrintf(WHITE, "请尝试 “%s --help” 获得帮助信息！\n", selfPath);
	}
}

void ShowVersion()
{
	printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
}

int DumpIMGFromISO(const File *mapFile, const char *path)
{
	int retVal = FAIL;

	File entry;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && path)
	{
		ColorPrintf(WHITE, "正在寻找IMG入口\t\t\t\t\t");
		if(GetBootEntryFromISO(mapFile, &entry) == PROCESS_SUCCESS)
		{
			ColorPrintf(LIME, "OK\n");
			ColorPrintf(WHITE, "正在写入IMG文件\t\t\t\t\t");
			if(WriteIMGToFile(&entry, path) == PROCESS_SUCCESS)
			{
				ColorPrintf(LIME, "OK\n");
				ColorPrintf(WHITE, "处理完毕\n");
				retVal = SUCCESS;//写入成功！
			}
			else
				ColorPrintf(RED, "FAIL\n");
		}
		else
			ColorPrintf(RED, "FAIL\n");
	}

	return retVal;
}

int DisplayInfoOfIMG(const File *mapFile)
{
	int retVal = FAIL;
	//todo
	return retVal;
}

int defaultOption(int argc, char **argv)
{
	int retVal = FAIL;

	File mapFile;

	if(argc != 2 || MapFile(&mapFile, argv[1]) != PROCESS_SUCCESS)
		ColorPrintf(RED, "输入文件：%s 映射失败！\n", argv[1]);
	else
	{
		ColorPrintf(WHITE, "%s", "检测到输入文件为：");

		switch(GetInputType(&mapFile))
		{
		case ISO:
			ColorPrintf(LIME, "%s", "ISO");
			ColorPrintf(WHITE, "，默认作为Acronis启动ISO处理：\n\n");
			retVal = DumpIMGFromISO(&mapFile, GetOutPath(argv[1], ".img"));
			//TestISO(&mapFile);//ISOProcess测试用例
			break;
		case IMG:
			ColorPrintf(LIME, "%s", "IMG");
			ColorPrintf(WHITE, "，默认显示IMG磁盘映像规格：\n\n");
			retVal = DisplayInfoOfIMG(&mapFile);
			//TestImage(&mapFile);
			break;
		default:
			ColorPrintf(RED, "%s\n", "未知");
		}

		UnmapFile(mapFile.pvFile);
	}

	return retVal;
}

int main(int argc, char **argv)
{
	//TestDebugFlag();//测试debug宏
	//TestSafeMemory();//测试SafeMemory模块
	//TestColorPrint();//测试彩色输出
	printf("刘宝修改了GetImage.c，演示冲突处理\n");
	ShowTitle();
	{
		int argIndex;
		struct Arg_parser parser;
		const struct ap_Option options[] =
		{
			/*!
				短选项超过unsigned char范围则认为无短选项，
				长选项为NULL则认为无长选项，
				ap_yes代表（如：-a）选项后面跟有参数，
				ap_no代表无参数，ap_maybe代表参数可有可无
			*/
			/*短选项，	对应长参数，	参数*/
			{'f',		"file",		ap_yes},
			{'d',		"device",	ap_yes},
			{'h',		"help",		ap_no},
			{'v',		"version",	ap_no},
			{0,			0,			ap_no}
		};

		if(!ap_init(&parser, argc, (const char * const *)argv, options, 0))
		{
			ColorPrintf(RED, "内存耗尽！\n");
			PRESS_ANY_KEY_AND_CONTINUE;
			return -1;
		}

		if(ap_error(&parser))
		{
			ShowError(ap_error(&parser), argv[0]);
			PRESS_ANY_KEY_AND_CONTINUE;
			return -1;
		}

		if(!ap_arguments(&parser))
			ShowHelp();

		for(argIndex = 0; argIndex < ap_arguments(&parser); ++argIndex)
		{
			const int code = ap_code(&parser, argIndex);
			const char *arg = ap_argument(&parser, argIndex);

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
			case 'v':
				ShowVersion();
				break;
			default:
				if(!argIndex)
					defaultOption(argc, argv);
			}

			if(!code) break;
		}

	}

	PRESS_ANY_KEY_AND_CONTINUE;
	return 0;
}

