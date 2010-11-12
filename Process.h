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

const char *GetOutPath(const char *selfPath, const char *extention);

MEDIA_TYPE GetInputType(const File *mapFile);

int WriteIMGToFile(const File *entry, const char *selfPath);

int GetBootEntryFromISO(const File *mapFile, File *entry);

void TestISO(const File *mapFile);

void TestImage(const File *mapFile);

#endif