/*!
\file ColorPrint.h
\author LiuBao
\version 1.0
\date 2010/12/28
\brief Windows����̨��ɫ����ӿڼ�ɫ�ʶ���
*/
#ifndef COLOR_PRINT
#define COLOR_PRINT

typedef enum _COLOR	///  ɫ�ʶ���
{
	BLACK,			///< ��ɫ
	NAVY,			///< ����ɫ
	GREEN,			///< ��ɫ
	TEAL,			///< ��ɫ
	MAROON,			///< �ֺ�ɫ
	PURPLE,			///< ��ɫ
	OLIVE,			///< �����
	SILVER,			///< ��ɫ
	GRAY,			///< ��ɫ
	BLUE,			///< ��ɫ
	LIME,			///< ����ɫ
	AQUA,			///< ǳ��ɫ
	RED,			///< ��ɫ
	FUCHSIA,		///< �Ϻ�ɫ
	YELLOW,			///< ��ɫ
	WHITE			///< ��ɫ
}COLOR;

/*!
����̨��ɫ�������printf������������ɫѡ��
\param color �����ɫ
\param format �����ʽ��ͬprintf
\return д����ַ������������ظ�ֵ��ͬprintf
*/
int ColorPrintf(COLOR color, const char *format, ...);

/*!
����ɫ����ӡ����ɫ�ʣ����ڲ���ɫ�����
*/
void PrintAllColor();

#endif