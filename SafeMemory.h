#ifndef SAFE_MEMORY
#define SAFE_MEMORY

#include <stddef.h>		/*!< ʹ��size_t */

#define MEM_DETAIL 0	/*!< �ڴ����������Ϣ����꣬��0�ر��������1������� */

/*! ����malloc��free */
#define malloc ��Ҫֱ�ӵ���malloc!
#define free ��Ҫֱ�ӵ���free!

#define MALLOC(size) Mem_alloc(size, __FILE__, __LINE__)
#define NEW(p) ((p) = MALLOC(sizeof *(p)))
#define FREE(p) Mem_free(p, __FILE__, __LINE__)

/*!
��ȫ�ڴ���䣬���Ƽ�ֱ�ӵ��ã���ʹ��MALLOC��NEW
\param size �����ڴ��С
\param file �����ڴ����Ĵ����ļ�
\param line �����ڴ����Ĵ�����
*/
void *Mem_alloc(size_t size, const char *file, int line);

/*!
��ȫ�ڴ��ͷź��������Ƽ�ֱ�ӵ��ã���ʹ��FREE
\param ptr �ڴ��ָ��
\param file �����ڴ��ͷŵĴ����ļ�
\param line �����ڴ��ͷŵĴ�����
*/
void Mem_free(void *ptr, const char *file, int line);

#endif
