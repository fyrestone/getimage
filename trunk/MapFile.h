#ifndef MAP_FILE
#define MAP_FILE

#include <stdio.h>		/*!< FILE */
#include "IntTypes.h"

#define MEDIA_CUR 0		/*!< ��ǰλ�� */
#define MEDIA_SET 1		/*!< ��ʼλ�� */

#define T media_t		/*!< �����������ͣ�ADT��media_t */

typedef struct T *T;

typedef struct media_access
{
	const unsigned char *begin;
	uint32_t len;
}media_access;

/*!
�򿪽��ʣ�ʹ���ڴ�ӳ��
\param path ����·��
\param viewSize ��ͼ��С
\return ���ɹ���������Чmedia_t�����򷵻�NULL
*/
T OpenMedia(const char *path, uint32_t viewSize);

/*!
��λ�ø�λ
\param media ��Ҫ��λ��media_t
\return �ɹ�����SUCCESS�����򷵻�FAILED
*/
int RewindMedia(T media);

/*!
��ת��λ��
\param media ��Ҫ��ת��media_t
\param offset ��תƫ����������Ϊ������ǰ��ת���򸺣������ת��
\param base ����ȡֵΪMEDIA_CUR������ڵ�ǰλ�ã���MEDIA_SET���������ʼλ�ã�
\return �ɹ�����SUCCESS�����򷵻�FAILED
*/
int SeekMedia(T media, int64_t offset, int base);

/*!
�رս���
\param media ��Ҫ�رյ�media_t��ָ��
*/
void CloseMedia(T *media);

/*!
�ӵ�ǰmedia_tλ�û��ָ�����ȵ��ڴ��ֻ��ָ��
\param media ��Ҫ��ȡ���ʽṹ��media_t
\param access ���ʽṹ��ָ��
\param len �ӵ�ǰλ�����ɷ��ʳ���
\return �ɹ���ȡ����SUCCESS�����򷵻�FAILED
*/
int GetMediaAccess(T media, media_access *access, uint32_t len);


/*!
�ӵ�ǰmedia_tλ�ó�ȡsize��С�����fp��
\param media ��Ҫ��ȡ���ݵ�media_t
\param fp ���ָ�룬����stdout���߿�д���ļ�ָ��
\param size ��ȡ��С
\return ��ȡ�ɹ�����SUCCESS�����򷵻�FAILED
*/
int DumpMedia(T media, FILE *fp, int64_t size);

#ifdef _DEBUG
	/*!
	��Ԫ��������
	\param path ���ļ�·��
	*/
	void MapTestUnit(const char *path);
#endif

#undef T
#endif