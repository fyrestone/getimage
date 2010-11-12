#include <stdlib.h>		/*< malloc/free */
#include <memory.h>		/*< memset */
#include <assert.h>		/*< assert */
#include "SafeMemory.h"

#undef malloc//只有这里可以使用malloc。
#undef free//只有这里可以使用free。

#define GarbageByte 0xCC//垃圾字符。
#define EndByte 0xE1//结尾占位符，检测是否越界。


int SafeMalloc(Malloc *ptr, size_t size)
{
	void *mallocPtr = NULL;

	assert(ptr && size);

#ifdef _DEBUG
	mallocPtr = malloc(size + 1);//多一个结尾占位符空间。
#else
	mallocPtr = malloc(size);
#endif // _DEBUG

	if(mallocPtr)
	{
#ifdef _DEBUG
		memset(mallocPtr, GarbageByte, size);
		*((char*)mallocPtr + size) = (char)EndByte;
#endif // _DEBUG

		ptr->mallocPtr = mallocPtr;
		ptr->mallocSize = size;

		return SafeMallocSuccess;
	}
	else
		return SafeMallocFailed;
}

void SafeFree(Malloc *ptr)
{
	assert(ptr);
	assert(*((char*)ptr->mallocPtr + ptr->mallocSize)
		== (char)EndByte);//检测是否越界。

#ifdef _DEBUG
	memset(ptr->mallocPtr, GarbageByte, ptr->mallocSize);
#endif // _DEBUG

	free(ptr->mallocPtr);

	ptr->mallocPtr = NULL;
	ptr->mallocSize = 0;
}
