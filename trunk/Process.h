#ifndef PROCESS
#define PROCESS

#include "MapFile.h"

#define PROCESS_SUCCESS 0
#define PROCESS_FAILED -1

/*! ���ʸ�ʽ���� */
typedef int MEDIA_TYPE;

#define UNKNOWN 0x0			/*!< �κ�δ֪���� */
#define IMG 0x1				/*!< IMG��ʽ */
#define ISO 0x2				/*!< ISO��ʽ */

MEDIA_TYPE GetInputType(media_t media);

int DumpIMGFromISO(media_t media, const char *path);

int DisplayISOInfo(media_t media);

int DisplayIMGInfo(media_t media);

#endif