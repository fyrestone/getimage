/*!
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

/// 按任意键退出
#define PRESS_ANY_KEY_AND_CONTINUE     \
    do                                 \
    {                                  \
        ColorPrintf(WHITE, _T("."));   \
        _getch();                      \
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
    _tprintf(_T("\n%s %s\t版权所有(c) 2009-2010 刘宝 %s\n\n"), PROGRAM_NAME, PROGRAM_VERSION, _T(__DATE__));
}

/*!
打印帮助信息
*/
static void ShowHelp()
{
    _tprintf(_T("用法：\tgetimage -i <文件/设备> -o <输出路径>\n"));
    _tprintf(_T("选项：\n"));
    _tprintf(_T("\t-h, --help\t\t显示本帮助\n"));
    _tprintf(_T("\t-v, --version\t\t显示本程序名及版本号\n"));
    _tprintf(_T("\t-i, --input\t\t输入文件/设备路径\n"));
    _tprintf(_T("\t-o, --output\t\t输出路径\n"));
    _tprintf(_T("默认：\n当无选项时将输入的第一个参数作为文件处理：\n")
             _T("若为ISO，则自动提取其中的启动磁盘映像到ISO文件所在目录，生成的IMG与ISO同名；\n")
             _T("若为IMG，则显示其规格信息\n"));
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
            ColorPrintf(WHITE, _T("，默认作为Acronis启动ISO处理：\n\n"));
            if(DisplayISOInfo(media) == SUCCESS)
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

    setlocale(LC_ALL, "Chinese_People's Republic of China.936");

    ShowTitle();//打印版权头

    /* 初始化参数解析 */
    if(!ap_init(&parser, argc, argv, options, 0))
    {
        ColorPrintf(RED, _T("内存耗尽！\n"));
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

    {
        const _TCHAR *input = NULL, *output = NULL;

        /* 遍历解析出的参数 */
        for(argIndex = 0; argIndex < ap_arguments(&parser); ++argIndex)
        {
            const int code = ap_code(&parser, argIndex);//解析出的短选项
            const _TCHAR *arg = ap_argument(&parser, argIndex);//解析出的选项对应的参数

            switch(code)
            {
            case 'i':
                input = arg;
                break;
            case 'o':
                output = arg;
                break;
            case 'h':
                ShowHelp();
                break;
            case 'v'://啥都不做
                break;
            default:
                //当前仅当argv[1]无法识别为有效选项，且只有一个运行参数时
                if(!argIndex && argc == 2)
                {
                    const _TCHAR *output = GetOutPath(argv[1], _T(".img"));
                    
                    if(output)//获取输出路径成功
                        GetImage(argv[1], output);
                    else//获取输出路径失败
                        ColorPrintf(RED, _T("输出路径为空！\n"));
                }
            }

            if(!code) break;
        }

        if(input && !output)
            ShowError(_T("无输出路径"), argv[0]);
        else if(!input && output)
            ShowError(_T("无输入路径"), argv[0]);
        else if(input && output)
            GetImage(input, output);
    }

    /* 销毁参数解析 */
    ap_free(&parser);

    PRESS_ANY_KEY_AND_CONTINUE;
    return 0;
}

