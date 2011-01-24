/*!
\file SafeMemory.h
\author LiuBao
\version 1.0
\date 2010/12/25
\brief ��ȫ���ڴ�����ӿ�MALLOC/NEW/FREE
*/
#ifndef SAFE_MEMORY
#define SAFE_MEMORY

#include <stddef.h>		/* ʹ��size_t */

/// �ڴ����������Ϣ���ٺ꣬��0�ر��������1�����������debug����Ч��
#define MEM_DETAIL 1	

#define malloc ��Ҫֱ�ӵ���malloc!	///< ����malloc	
#define free ��Ҫֱ�ӵ���free!		///< ����free

/// ��̬����size��С�ĵĿռ䣬�൱��malloc
#define MALLOC(size) Mem_alloc(size, __FILE__, __LINE__)
/// Ϊ��֪����ָ�붯̬�������ʹ�С�ռ�
#define NEW(p) ((p) = MALLOC(sizeof *(p)))
/// �ͷ�MALLOC��NEW����Ŀռ�
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
