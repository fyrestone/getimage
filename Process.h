/*!
\file Process.h
\author LiuBao
\version 1.0
\date 2011/1/23
\brief ����������
*/
#ifndef PROCESS
#define PROCESS

#include "MapFile.h"

typedef int MEDIA_TYPE;	///< ���ʸ�ʽ����

#define UNKNOWN 0x0		///< �κ�δ֪����
#define IMG 0x1			///< IMG��ʽ
#define ISO 0x2			///< ISO��ʽ	

/*!
��ý������ͣ�������UNKNOWN��IMG��ISO
\param media λ���ڽ���ͷ����media_t
\return �������ͣ�UNKNOWN��IMG��ISO��
*/
MEDIA_TYPE GetInputType(media_t media);

/*!
��ISO����ȡ������IMGӳ�񣬱�����ISO�ļ�����·������չ��Ϊ.img
\param media λ����ISOͷ����media_t
\param path ISO�ļ�����·��
\return ��ȡ�ɹ�����SUCCESS�����򷵻�FAILED
*/
int DumpIMGFromISO(media_t media, const char *path);

/*!
���ISO�����Ϣ
\param media λ����ISOͷ����media_t
\return �ɹ�����SUCCESS�����򷵻�FAILED
*/
int DisplayISOInfo(media_t media);

/*!
���IMG�����Ϣ
\param media λ����IMGͷ����media_t
\return �ɹ�����SUCCESS�����򷵻�FAILED
*/
int DisplayIMGInfo(media_t media);

#endif