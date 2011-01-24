/*!
\file SafeMemory.c
\author LiuBao
\version 1.0
\date 2010/12/25
\brief ��ȫ�ڴ����ʵ��
*/
#include <stdlib.h>		/* malloc/free */
#include <memory.h>		/* memset */
#include <assert.h>		/* assert */
#include "ColorPrint.h"	/* ColorPrintf */
#include "SafeMemory.h"

#undef malloc	/* ֻ���������ʹ��malloc */
#undef free		/* ֻ���������ʹ��free */

#define GarbageByte 0xCC	///< �����ַ�
#define EndByte 0xE1		///< ��βռλ��������Ƿ�Խ��

void *Mem_alloc(size_t size, const char *file, int line)
{
	void *mallocPtr = NULL;

	assert(size);

#ifdef _DEBUG
	if((mallocPtr = malloc(size + sizeof(size_t) + 1)))//��һ����βռλ���ռ䡣
	{
	#if MEM_DETAIL
		ColorPrintf(YELLOW, "�ڴ���䣨%p����%s��%d->%u\n", mallocPtr, file, line, size);
	#endif
		*(size_t *)(mallocPtr) = size;						/* ����malloc��С */
		mallocPtr = (char *)(mallocPtr) + sizeof(size_t);	/* ���¼���mallocָ�� */
		memset(mallocPtr, GarbageByte, size);				/* ������������д */
		*((char *)mallocPtr + size) = (char)EndByte;		/* д���βռλ�� */
	}
#else
	mallocPtr = malloc(size);
#endif // _DEBUG

	if(!mallocPtr)
	{
		ColorPrintf(RED, "�ڴ����ʧ�ܣ�\n");
		ColorPrintf(YELLOW, "��С��%u\n", size);
		ColorPrintf(YELLOW, "�ļ���%s\n", file);
		ColorPrintf(YELLOW, "�кţ�%d\n", line);
	}

	return mallocPtr;
}

void Mem_free(void *ptr, const char *file, int line)
{
#ifdef _DEBUG
	void *mallocPtr = NULL;	/* ʵ��mallocָ�� */
	size_t mallocSize = 0;	/* ʵ��malloc��С */

	assert(ptr);

	/* ����ʵ��mallocָ�뼰malloc�ռ��С */
	mallocPtr = (char *)ptr - sizeof(size_t);
	mallocSize = *(size_t *)(mallocPtr);

	assert(*((char*)ptr + mallocSize) == (char)EndByte);//����Ƿ�Խ�硣

	/* ������������д */
	memset(ptr, GarbageByte, mallocSize);

	#if MEM_DETAIL
		ColorPrintf(YELLOW, "�ڴ����٣�%p����%s��%d->%u\n", mallocPtr, file, line, mallocSize);
	#endif

	free(mallocPtr);
#else
	free(ptr);
#endif // _DEBUG
}
