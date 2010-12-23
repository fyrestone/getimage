#ifndef SAFE_MEMORY
#define SAFE_MEMORY

#include <stddef.h>		/*!< 使用size_t */

#define MEM_DETAIL 0	/*!< 内存分配销毁信息输出宏，置0关闭输出，置1开启输出 */

/*! 禁用malloc和free */
#define malloc 不要直接调用malloc!
#define free 不要直接调用free!

#define MALLOC(size) Mem_alloc(size, __FILE__, __LINE__)
#define NEW(p) ((p) = MALLOC(sizeof *(p)))
#define FREE(p) Mem_free(p, __FILE__, __LINE__)

/*!
安全内存分配，不推荐直接调用，请使用MALLOC、NEW
\param size 分配内存大小
\param file 调用内存分配的代码文件
\param line 调用内存分配的代码行
*/
void *Mem_alloc(size_t size, const char *file, int line);

/*!
安全内存释放函数，不推荐直接调用，请使用FREE
\param ptr 内存块指针
\param file 调用内存释放的代码文件
\param line 调用内存释放的代码行
*/
void Mem_free(void *ptr, const char *file, int line);

#endif
