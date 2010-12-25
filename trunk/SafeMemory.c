/*!
\file SafeMemory.c
\author LiuBao
\version 1.0
\date 2010/12/25
\brief 安全内存操作实现
*/
#include <stdlib.h>		/*< malloc/free */
#include <memory.h>		/*< memset */
#include <assert.h>		/*< assert */
#include "ColorPrint.h"
#include "SafeMemory.h"

#undef malloc//只有这里可以使用malloc。
#undef free//只有这里可以使用free。

#define GarbageByte 0xCC//垃圾字符。
#define EndByte 0xE1//结尾占位符，检测是否越界。

void *Mem_alloc(size_t size, const char *file, int line)
{
	void *mallocPtr = NULL;

	assert(size);

#ifdef _DEBUG
	if((mallocPtr = malloc(size + sizeof(size_t) + 1)))//多一个结尾占位符空间。
	{
#if MEM_DETAIL
		ColorPrintf(YELLOW, "内存分配（%p）：%s：%d->%u\n", mallocPtr, file, line, size);
#endif
		*(size_t *)(mallocPtr) = size;
		mallocPtr = (char *)(mallocPtr) + sizeof(size_t);
		memset(mallocPtr, GarbageByte, size);
		*((char *)mallocPtr + size) = (char)EndByte;
	}
#else
	mallocPtr = malloc(size);
#endif // _DEBUG

	if(!mallocPtr)
	{
		ColorPrintf(RED, "内存分配失败：\n");
		ColorPrintf(YELLOW, "大小：%u\n", size);
		ColorPrintf(YELLOW, "文件：%s\n", file);
		ColorPrintf(YELLOW, "行号：%d\n", line);
	}

	return mallocPtr;
}

void Mem_free(void *ptr, const char *file, int line)
{
#ifdef _DEBUG
	void *mallocPtr = NULL;
	size_t mallocSize = 0;

	assert(ptr);

	mallocPtr = (char *)ptr - sizeof(size_t);
	mallocSize = *(size_t *)(mallocPtr);

	assert(*((char*)ptr + mallocSize) == (char)EndByte);//检测是否越界。

	memset(ptr, GarbageByte, mallocSize);
#if MEM_DETAIL
	ColorPrintf(YELLOW, "内存销毁（%p）：%s：%d->%u\n", mallocPtr, file, line, mallocSize);
#endif

	free(mallocPtr);
#else
	free(ptr);
#endif // _DEBUG
}
