/*!
\file SafeMemory.c
\author LiuBao
\version 1.0
\date 2010/12/25
\brief 安全内存操作实现
*/
#include <stdlib.h>        /* malloc/free */
#include <memory.h>        /* memset */
#include <assert.h>        /* assert */
#include "ColorPrint.h"    /* ColorPrintf */
#include "SafeMemory.h"

#undef malloc      /* 只有这里可以使用malloc */
#undef free        /* 只有这里可以使用free */

#define GarbageByte 0xCC   ///< 垃圾字符
#define EndByte 0xE1       ///< 结尾占位符，检测是否越界

void *Mem_alloc(size_t size, const _TCHAR *file, int line)
{
    void *mallocPtr = NULL;

    assert(size);

#ifdef _DEBUG
    if((mallocPtr = malloc(size + sizeof(size_t) + 1)))//多一个结尾占位符空间。
    {
    #if MEM_DETAIL
        ColorPrintf(YELLOW, _T("内存分配（%p）：%s：%d->%u\n"), mallocPtr, file, line, size);
    #endif
        *(size_t *)(mallocPtr) = size;                       /* 储存malloc大小 */
        mallocPtr = (char *)(mallocPtr) + sizeof(size_t);    /* 重新计算malloc指针 */
        memset(mallocPtr, GarbageByte, size);                /* 用垃圾数据重写 */
        *((char *)mallocPtr + size) = (char)EndByte;         /* 写入结尾占位符 */
    }
#else
    mallocPtr = malloc(size);
#endif // _DEBUG

    if(!mallocPtr)
    {
        ColorPrintf(RED, _T("内存分配失败：\n"));
        ColorPrintf(YELLOW, _T("大小：%u\n"), size);
        ColorPrintf(YELLOW, _T("文件：%s\n"), file);
        ColorPrintf(YELLOW, _T("行号：%d\n"), line);
    }

    return mallocPtr;
}

void Mem_free(void *ptr, const _TCHAR *file, int line)
{
#ifdef _DEBUG
    void *mallocPtr = NULL;   /* 实际malloc指针 */
    size_t mallocSize = 0;    /* 实际malloc大小 */

    if(ptr)
    {
        /* 计算实际malloc指针及malloc空间大小 */
        mallocPtr = (char *)ptr - sizeof(size_t);
        mallocSize = *(size_t *)(mallocPtr);

        assert(*((char*)ptr + mallocSize) == (char)EndByte);//检测是否越界。

        /* 用垃圾数据重写 */
        memset(ptr, GarbageByte, mallocSize);

    #if MEM_DETAIL
        ColorPrintf(YELLOW, _T("内存销毁（%p）：%s：%d->%u\n"), mallocPtr, file, line, mallocSize);
    #endif

        free(mallocPtr);
    }
    else
    {
    #if MEM_DETAIL
        ColorPrintf(YELLOW, _T("空指针销毁：%s：%d\n"), file, line);
    #endif
    }
#else
    free(ptr);
#endif // _DEBUG
}
