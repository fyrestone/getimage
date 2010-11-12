#include <Windows.h>
#include <assert.h>
#include "MapFile.h"

int MapFile(File *mapFile, const char *file)
{
	int retVal = MAP_FAILED;

	HANDLE hFile, hFileMapping;
	PVOID pvFile;
	uint32_t fileSize;

	assert(mapFile && file);

	hFile = CreateFileA(
		file, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hFile != INVALID_HANDLE_VALUE)
	{

		fileSize = GetFileSize(hFile, NULL);//获得文件大小
		hFileMapping = CreateFileMapping(
			hFile, NULL, PAGE_READONLY, 0, fileSize, NULL);

		CloseHandle(hFile);
		if(hFileMapping)
		{
			pvFile = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

			CloseHandle(hFileMapping);
			if(pvFile)
			{
				mapFile->pvFile = pvFile;
				mapFile->size = fileSize;
				retVal = MAP_SUCCESS;//映射成功
			}
		}
	}

	return retVal;
}

void UnmapFile(const void *pvFile)
{
	(void)UnmapViewOfFile((PVOID)pvFile);
}