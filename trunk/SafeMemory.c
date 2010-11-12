#include <stdlib.h>		/*< malloc/free */
#include <memory.h>		/*< memset */
#include <assert.h>		/*< assert */
#include "SafeMemory.h"

#undef malloc//ֻ���������ʹ��malloc��
#undef free//ֻ���������ʹ��free��

#define GarbageByte 0xCC//�����ַ���
#define EndByte 0xE1//��βռλ��������Ƿ�Խ�硣


int SafeMalloc(Malloc *ptr, size_t size)
{
	void *mallocPtr = NULL;

	assert(ptr && size);

#ifdef _DEBUG
	mallocPtr = malloc(size + 1);//��һ����βռλ���ռ䡣
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
		== (char)EndByte);//����Ƿ�Խ�硣

#ifdef _DEBUG
	memset(ptr->mallocPtr, GarbageByte, ptr->mallocSize);
#endif // _DEBUG

	free(ptr->mallocPtr);

	ptr->mallocPtr = NULL;
	ptr->mallocSize = 0;
}
