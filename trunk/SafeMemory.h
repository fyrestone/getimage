#ifndef SAFE_MEMORY
#define SAFE_MEMORY

#include <stddef.h>		/*!< 使用size_t */

/*! 禁用malloc和free */
#define malloc 不要直接调用malloc!
#define free 不要直接调用free!

#define SafeMallocSuccess 0
#define SafeMallocFailed -1

typedef struct
{
    void *mallocPtr;
    size_t mallocSize;
} Malloc;

/*! 安全内存分配函数，分配成功返回SafeMallocSuccess，失败返回SafeMallocFailed。 */
int SafeMalloc(Malloc *ptr, size_t size);

/*! 安全内存释放函数。 */
void SafeFree(Malloc *ptr);

#endif
