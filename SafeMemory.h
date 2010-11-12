#ifndef SAFE_MEMORY
#define SAFE_MEMORY

#include <stddef.h>		/*!< ʹ��size_t */

/*! ����malloc��free */
#define malloc ��Ҫֱ�ӵ���malloc!
#define free ��Ҫֱ�ӵ���free!

#define SafeMallocSuccess 0
#define SafeMallocFailed -1

typedef struct
{
    void *mallocPtr;
    size_t mallocSize;
} Malloc;

/*! ��ȫ�ڴ���亯��������ɹ�����SafeMallocSuccess��ʧ�ܷ���SafeMallocFailed�� */
int SafeMalloc(Malloc *ptr, size_t size);

/*! ��ȫ�ڴ��ͷź����� */
void SafeFree(Malloc *ptr);

#endif
