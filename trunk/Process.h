#ifndef PROCESS
#define PROCESS

#include "MapFile.h"

#define PROCESS_SUCCESS 0
#define PROCESS_FAILED -1

/*! 介质格式定义 */
typedef int MEDIA_TYPE;

#define UNKNOWN 0x0			/*!< 任何未知类型 */
#define IMG 0x1				/*!< IMG格式 */
#define ISO 0x2				/*!< ISO格式 */

MEDIA_TYPE GetInputType(const File *mapFile);

int DumpIMGFromISO(const File *mapFile, const char *path);

int DisplayISOInfo(const File *mapFile);

int DisplayIMGInfo(const File *mapFile);

void TestISO(const File *mapFile);

void TestIMG(const File *mapFile);

#endif