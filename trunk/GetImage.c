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


void TestDebugFlag()
{
	ColorPrintf(AQUA, "��ʼ����debug�꣺\n");

#ifdef _DEBUG
	puts("��ǰ��Ϊ��_DEBUG");
#endif
#ifdef NDEBUG
	puts("��ǰ��Ϊ��NDEBUG");
#endif
}

void ShowTitle()
{
	printf("\n%s %s\t��Ȩ����(c) 2009-2010 ���� %s\n\n", PROGRAM_NAME, PROGRAM_VERSION, __DATE__);
}

void ShowHelp()
{
	printf("�÷���\tgetimage [ѡ��] �ļ����豸\n");
	printf("ѡ�\n");
	printf("\t-h, --help\t\t��ʾ������\n");
	printf("\t-v, --version\t\t��ʾ�����������汾��\n");
	printf("\t-f, --file\t\t��һ���ļ�\n");
	printf("\t-d, --device\t\t��һ���豸\n");
	printf("���⣺\n\t����ѡ��ʱ��Ĭ�Ͻ�����ĵ�һ��������Ϊ�ļ�����"
		"��ΪISO�����Զ���ȡ���е���������ӳ��ISO�ļ�����Ŀ¼�����ɵ�IMG��ISOͬ����"
		"��ΪIMG������ʾ������Ϣ��������ʾ������Ϣ��\n");
}

void ShowError(const char *msg, const char *selfPath)
{
	if(msg && msg[0])
	{
		ColorPrintf(RED, "%s��%s\n", PROGRAM_NAME, msg);
		ColorPrintf(WHITE, "�볢�� ��%s --help�� ��ð�����Ϣ��\n", selfPath);
	}
}

void ShowVersion()
{
	printf("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
}

int DefaultOption(int argc, char **argv)
{
	int retVal = FAIL;

	media_t media;

	if(argc != 2 || !(media = OpenMedia(argv[1], 128000)))
		ColorPrintf(RED, "�����ļ���%s ӳ��ʧ�ܣ�\n", argv[1]);
	else
	{
		ColorPrintf(WHITE, "%s", "��⵽�����ļ�Ϊ");

		switch(GetInputType(media))
		{
		case ISO:
			ColorPrintf(LIME, "%s", "ISO");
			ColorPrintf(WHITE, "��Ĭ����ΪAcronis����ISO����\n\n");
			DisplayISOInfo(media);
			retVal = DumpIMGFromISO(media, argv[1]);
			//TestISO(&mapFile);//ISOProcess��������
			break;
		case IMG:
			ColorPrintf(LIME, "%s", "IMG");
			ColorPrintf(WHITE, "��Ĭ����ʾIMG����ӳ����\n\n");
			retVal = DisplayIMGInfo(media);
			//TestIMG(&mapFile);
			break;
		default:
			ColorPrintf(RED, "%s\n", "δ֪����");
		}

		CloseMedia(&media);
	}

	return retVal;
}

int main(int argc, char **argv)
{
	//TestDebugFlag();//����debug��
	{
		//MapTestUnit(argv[1]);
		//media_t media = OpenMedia(argv[1], 0x111);
		//ColorPrintf(RED, "media:%p\n", media);
	}

	ShowTitle();
	{
		int argIndex;
		struct Arg_parser parser;
		const struct ap_Option options[] =
		{
			/*!
				��ѡ���unsigned char��Χ����Ϊ�޶�ѡ�
				��ѡ��ΪNULL����Ϊ�޳�ѡ�
				ap_yes�����磺-a��ѡ�������в�����
				ap_no�����޲�����ap_maybe����������п���
			*/
			/*��ѡ�	��Ӧ��������	����*/
			{'f',		"file",		ap_yes},
			{'d',		"device",	ap_yes},
			{'h',		"help",		ap_no},
			{'v',		"version",	ap_no},
			{0,			0,			ap_no}
		};

		if(!ap_init(&parser, argc, (const char * const *)argv, options, 0))
		{
			ColorPrintf(RED, "�ڴ�ľ���\n");
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
				printf("��⵽����Ϊ��%s\n", arg);
				break;
			case 'd':
				printf("��⵽����Ϊ��%s\n", arg);
				break;
			case 'h':
				ShowHelp();
				break;
			case 'v':
				ShowVersion();
				break;
			default:
				if(!argIndex)
					DefaultOption(argc, argv);
			}

			if(!code) break;
		}

	}

	PRESS_ANY_KEY_AND_CONTINUE;
	return 0;
}

