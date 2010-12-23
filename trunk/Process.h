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

MEDIA_TYPE GetInputType(media_t media);

int DumpIMGFromISO(media_t media, const char *path);

int DisplayISOInfo(media_t media);

int DisplayIMGInfo(media_t media);

#endif