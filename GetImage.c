﻿/*!
\file GetImage.c
\author LiuBao
\version 1.0
\date 2011/1/23
\brief GetImage的main函数及针对不同参数的处理函数
*/
#include <stdio.h>
#include <conio.h>                    /* 使用_getch */
#include <locale.h>                   /* 使用setlocale */
#include "ProjDef.h"
#include "ArgParser\carg_parser.h"    /* 使用ArgParser参数解析 */
#include "ColorPrint.h"
#include "Process.h"

#define PROGRAM_NAME _T("GETIMAGE")   ///< 程序名
#define PROGRAM_YEAR _T("2010")       ///< 年份
#define PROGRAM_VERSION _T("1.5")     ///< 版本

/// 按任意键退出，当且仅当父进程是explorer.exe时有效
#define PRESS_ANY_KEY_AND_CONTINUE          \
    do                                      \
    {                                       \
        if(isUnderExplorer() == SUCCESS)    \
        {                                   \
            ColorPrintf(WHITE, _T("."));    \
            _getch();                       \
        }                                   \
    }while(0)

/*!
测试Debug宏
*/
void TestDebugFlag()
{
    ColorPrintf(AQUA, _T("开始测试debug宏：\n"));

#ifdef _DEBUG
    _putts(_T("当前宏为：_DEBUG"));
#endif
#ifdef NDEBUG
    _putts(_T("当前宏为：NDEBUG"));
#endif
}

/*!
打印版权头
*/
static void ShowTitle()
{
    _tprintf(_T("\n%s %s\t版权所有(c) 2009-2011 zihongdelei@yahoo.com.cn %s\n\n"), PROGRAM_NAME, PROGRAM_VERSION, _T(__DATE__));
}

/*!
打印帮助信息
*/
static void ShowHelp()
{
    ColorPrintf(WHITE, _T("用法："));
    _tprintf(_T("\tgetimage -i <输入文件/设备路径> -o <输出路径>\n"));
    ColorPrintf(WHITE, _T("选项：\n"));
    _tprintf(_T("\t-h, --help\t\t显示本帮助\n"));
    _tprintf(_T("\t-v, --version\t\t显示本程序名及版本号\n"));
    _tprintf(_T("\t-i, --input\t\t输入文件/设备路径\n"));
    _tprintf(_T("\t-o, --output\t\t输出路径\n"));
    ColorPrintf(WHITE, _T("注意：\n"));
    _tprintf(_T("\tVista/Windows 7下打开本地磁盘分区，需要使用管理员权限运行本程序\n"));
    _tprintf(_T("\t本程序规格显示，仅支持IMG（FAT12/16/32）、ISO（标准映像）\n"));
    ColorPrintf(WHITE, _T("使用举例：\n"));
    ColorPrintf(WHITE, _T("\n1、getimage <输入文件路径>\n"));
    _tprintf(_T("   ├显示IMG文件规格；提取ISO文件的启动IMG映像到<ISO文件无扩展名路径 + .img>\n"));
    _tprintf(_T("   ├getimage E:\\test.iso 提取test.iso的启动IMG映像到E:\\test.img\n"));
    _tprintf(_T("   └getimage E:\\test.img 显示test.img的规格\n"));
    ColorPrintf(WHITE, _T("\n2、getimage <输入设备路径>\n"));
    _tprintf(_T("   ├显示（IMG/ISO）设备规格\n"));
    _tprintf(_T("   └getimage \\\\.\\H: 显示H盘的规格\n"));
    ColorPrintf(WHITE, _T("\n3、getimage -i <输入文件/设备路径>\n"));
    _tprintf(_T("   ├显示（IMG/ISO）文件/设备规格\n"));
    _tprintf(_T("   ├getimage E:\\test.iso 显示test.iso的规格\n"));
    _tprintf(_T("   ├getimage E:\\test.img 显示test.img的规格\n"));
    _tprintf(_T("   └getimage -i \\\\.\\H: 显示H盘的规格\n"));
    ColorPrintf(WHITE, _T("\n4、getimage -i <输入文件/设备路径> -o <输出路径>\n"));
    _tprintf(_T("   ├显示IMG文件/设备规格；提取ISO文件/设备的启动IMG映像到<输出路径>\n"));
    _tprintf(_T("   ├getimage -i \\\\.\\H: -o E:\\test.img\n"));
    _tprintf(_T("   └getimage -i E:\\test.iso -o E:\\test.img\n"));
}

/*!
打印版本信息
*/
static void ShowVersion()
{
    _tprintf(_T("版本：%s\n"), PROGRAM_VERSION);
}

/*!
打印错误信息
\param msg 错误信息字符串
\param selfPath 本程序绝对路径
*/
static void ShowError(const _TCHAR *msg, const _TCHAR *selfPath)
{
    if(msg && msg[0])
    {
        ColorPrintf(RED, _T("%s：%s\n"), PROGRAM_NAME, msg);
        ColorPrintf(WHITE, _T("请尝试 “%s --help” 获得帮助信息！\n"), selfPath);
    }
}

/*!
对input处理，若是ISO，则提取IMG到output；否则打印IMG规格信息
\param input 输入路径
\param output 输出路径
\return 处理成功，返回SUCCESS；否则返回FAILED
*/
static int GetImage(const _TCHAR *input, const _TCHAR *output)
{
    int retVal = FAILED;

    media_t media;

    if(!(media = OpenMedia(input, 128000)))
        ColorPrintf(RED, _T("输入文件：%s 打开失败！\n"), input);
    else
    {
        ColorPrintf(WHITE, _T("检测到输入文件为"));

        switch(GetInputType(media))
        {
        case ISO:
            ColorPrintf(LIME, _T("ISO"));
            ColorPrintf(WHITE, _T("，支持Acronis启动ISO：\n\n"));
            if(DisplayISOInfo(media) == SUCCESS)
                retVal = SUCCESS;
            if(output)
                retVal = DumpIMGFromISO(media, output);
            break;
        case IMG:
            ColorPrintf(LIME, _T("IMG"));
            ColorPrintf(WHITE, _T("，默认显示IMG磁盘映像规格：\n\n"));
            retVal = DisplayIMGInfo(media);
            break;
        default:
            ColorPrintf(RED, _T("未知类型\n"));
        }

        CloseMedia(&media);
    }

    return retVal;
}

int _tmain(int argc, _TCHAR **argv)
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

        /* 短选项，    对应长参数，    参数 */
        {'i',        _T("input"),     ap_yes},
        {'o',        _T("output"),    ap_yes},
        {'h',        _T("help"),      ap_no},
        {'v',        _T("version"),   ap_no},
        {0,          0,               ap_no}
    };

    /* 设置地域信息，改变wprintf */
    setlocale(LC_ALL, "Chinese_People's Republic of China.936");

    /* 打印版权头 */
    ShowTitle();

    /* 初始化参数解析 */
    if(!ap_init(&parser, argc, argv, options, 0))
    {
        ShowError(_T("内存耗尽"), argv[0]);
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
    else
    {
        const _TCHAR *input = NULL, *output = NULL;
        int iCount = 0, oCount = 0, hCount = 0, vCount = 0;

        /* 遍历解析出的参数 */
        for(argIndex = 0; argIndex < ap_arguments(&parser); ++argIndex)
        {
            const int code = ap_code(&parser, argIndex);//解析出的短选项
            const _TCHAR *arg = ap_argument(&parser, argIndex);//解析出的选项对应的参数

            switch(code)
            {
            case 'i':
                input = arg;
                ++iCount;
                break;
            case 'o':
                output = arg;
                ++oCount;
                break;
            case 'h':
                ++hCount;
                break;
            case 'v':
                ++vCount;
                break;
            default:
                break;
            }

            if(!code) break;
        }

        {
            int valid = 0;

            /* 
            getimage -i <输入路径>
            getimage -i <输入路径> -o <输出路径>
            */
            if(iCount == 1 && (oCount == 0 || oCount == 1) && !hCount && !vCount)
            {
                (void)GetImage(input, output);
                valid = 1;
            }
            /* getimage <输入路径> */
            else if(argc == 2 && !iCount && !oCount && !hCount && !vCount)
            {
                (void)GetImage(argv[1], GetOutPath(argv[1], _T(".img")));
                valid = 1;
            }
            /* getimage -v */
            else if(argc == 2 && vCount == 1)
            {
                ShowVersion();
                valid = 1;
            }

            if(!valid) ShowHelp();
        }
    }

    /* 销毁参数解析 */
    ap_free(&parser);

    PRESS_ANY_KEY_AND_CONTINUE;
    return 0;
}

