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

/*! 安全内存分配函数，分配成功返回指针。 */
void *Mem_alloc(size_t size, const char *file, int line);

/*! 安全内存释放函数。 */
void Mem_free(void *ptr, const char *file, int line);

#endif
