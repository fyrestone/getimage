#include <Windows.h>	/*!< 使用MAX_PATH宏 */
#include <stdio.h>		/*!< 使用FILE */
#include <assert.h>
#include "Process.h"
#include "ImageProcess.h"
#include "ISOProcess.h"

/*! 私有全局变量声明 */
static char PATH[MAX_PATH];

/*! 私有函数声明 */
static int WritePartToFile(const void *begin, size_t len, const char *outPath);

static int WritePartToFile(const void *begin, size_t len, const char *outPath)
{
	int retVal = PROCESS_FAILED;

	if(begin && len > 0 && outPath)
	{
		FILE *outfp = fopen(outPath, "wb");

		if(outfp && fwrite(begin, len, 1, outfp) == 1)
			retVal = PROCESS_SUCCESS;//写入成功
	}

	return retVal;
}

const char *GetOutPath(const char *selfPath, const char *extention)
{
	const char *retVal = NULL;

	const char *endOfSelfPath = selfPath + strlen(selfPath);

	while(*endOfSelfPath != '.' && endOfSelfPath != selfPath)
		--endOfSelfPath;

	if(endOfSelfPath != selfPath)
	{
		char *target = PATH;

		while(selfPath != endOfSelfPath)
			*target++ = *selfPath++;
	
		while((*target++  = *extention++));
		
		retVal = PATH;//获取输出路径
	}

	return retVal;
}

MEDIA_TYPE GetInputType(const File *mapFile)
{
	MEDIA_TYPE retVal = UNKNOWN;

	if(CHECK_FILE_PTR_IS_VALID(mapFile))
	{
		//检查是否为IMG
		if(CheckImageIdentifier(mapFile) == IMAGE_PROCESS_SUCCESS)
			retVal = IMG;//是IMG格式
		else
		{
			const PrimVolDesc *pcPVD = JumpToPrimVolDesc(mapFile);

			if(pcPVD && CheckISOIdentifier(pcPVD) == ISO_PROCESS_SUCCESS)
				retVal = ISO;//是ISO格式
		}
	}
	
	return retVal;
}

int WriteIMGToFile(const File *entry, const char *path)
{
	int retVal = PROCESS_FAILED;
	
	if(CHECK_FILE_PTR_IS_VALID(entry) && path)
	{
		const ImageBPB *pcBPB = (const ImageBPB *)(entry->pvFile);
		size_t totalSize = 0;

		{
			size_t totalSec = 0;

			if(LD_UINT16(pcBPB->Common.BPB_TotSec16))
				totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
			else
				totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

			totalSize = totalSec * LD_UINT16(pcBPB->Common.BPB_BytsPerSec);
		}

		if(totalSize && CHECK_PTR_SPACE_IS_VALID(entry, entry->pvFile, totalSize))
			retVal = WritePartToFile(entry->pvFile, totalSize, path);
	}

	return retVal;
}

int GetBootEntryFromISO(const File *mapFile, File *entry)
{
	int retVal = PROCESS_FAILED;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && entry)
	{
		const PrimVolDesc *pcPVD = NULL;
		const BootRecordVolDesc *pcBRVD = NULL;
		const ValidationEntry *pcVE = NULL;
		const InitialEntry *pcIE = NULL;

		entry->pvFile = NULL;
		entry->size = 0;

		/*连续跳转指针*/
		if((pcPVD = JumpToPrimVolDesc(mapFile)) &&
			(pcBRVD = JumpToBootRecordVolDesc(mapFile, pcPVD)) &&
			(pcVE = JumpToValidationEntry(mapFile, pcBRVD)) &&
			(pcIE = JumpToInitialEntry(mapFile, pcVE)))
		{
			const void *entryPtr = JumpToBootableImage(mapFile, pcIE);

			entry->pvFile = (void *)(entryPtr);
			entry->size = mapFile->size - ((char *)entryPtr - (char *)mapFile->pvFile);
		}

		/*检测跳转后的entry是否有效*/
		if(CHECK_FILE_PTR_IS_VALID(entry))
		{
			/*尝试从ISO的BootImage入口跳转到IMG入口*/
			if(CheckImageIdentifier(entry) != IMAGE_PROCESS_SUCCESS)
			{
				uint32_t offsetValue = LD_UINT8((char *)entry->pvFile + 244);
				uint32_t relativeOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
				const void *entryPtr = JUMP_PTR_BY_LENGTH(entry, entry->pvFile, relativeOffset);

				if(entryPtr)
				{
					entry->size = entry->size - ((char *)entryPtr - (char *)entry->pvFile);
					entry->pvFile = entryPtr;
				}
			}

			if(CHECK_FILE_PTR_IS_VALID(entry) &&							/*!< 指针有效 */
				(CheckImageIdentifier(entry) == IMAGE_PROCESS_SUCCESS) &&	/*!< 文件标识有效 */
				(CheckFileSystem(entry) == IMAGE_PROCESS_SUCCESS) &&		/*!< 文件系统检查 */
				(CHECK_PTR_SPACE_IS_VALID(entry, entry->pvFile, 512)))		/*!< BPB可读检查 */
				retVal = PROCESS_SUCCESS;//找到IMG入口
		}

	}

	return retVal;
}

void TestISO(const File *mapFile)
{
	ISOTestUnit(mapFile);
}

void TestImage(const File *mapFile)
{
	ImageTestUnit(mapFile);
}
