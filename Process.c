#include <Windows.h>	/*!< 使用MAX_PATH宏 */
#include <stdio.h>		/*!< 使用FILE */
#include <assert.h>
#include "ColorPrint.h"
#include "Process.h"
#include "IMGProcess.h"
#include "ISOProcess.h"

/*! 私有全局变量声明 */
static char PATH[MAX_PATH];
static const char * const leftMargin = "    ";

/*! 私有函数声明 */
static const char *GetOutPath(const char *selfPath, const char *extention);
static int WritePartToFile(const void *begin, size_t len, const char *outPath);
static int WriteIMGToFile(const File *entry, const char *selfPath);
static int GetBootEntryFromISO(const File *mapFile, File *entry);

static const char *GetOutPath(const char *path, const char *extention)
{
	const char *retVal = NULL;

	const char *endOfSelfPath = path + strlen(path);

	while(*endOfSelfPath != '.' && endOfSelfPath != path)
		--endOfSelfPath;

	if(endOfSelfPath != path)
	{
		char *target = PATH;

		while(path != endOfSelfPath)
			*target++ = *path++;
	
		while((*target++  = *extention++));
		
		retVal = PATH;//获取输出路径
	}

	return retVal;
}

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

static int WriteIMGToFile(const File *entry, const char *path)
{
	int retVal = PROCESS_FAILED;
	
	if(CHECK_FILE_PTR_IS_VALID(entry) && path)
	{
		const BPB *pcBPB = (const BPB *)(entry->pvFile);
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

static int GetBootEntryFromISO(const File *mapFile, File *entry)
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
		if((pcPVD = JumpToISOPrimVolDesc(mapFile)) &&
			(CheckISOIdentifier(pcPVD) == ISO_PROCESS_SUCCESS) &&		/*!< 检测ISO标记 */
			(pcBRVD = JumpToISOBootRecordVolDesc(mapFile, pcPVD)) &&
			(CheckISOBootIdentifier(pcBRVD) == ISO_PROCESS_SUCCESS) &&	/*!< 检测启动盘标记 */
			(pcVE = JumpToISOValidationEntry(mapFile, pcBRVD)) &&
			(pcIE = JumpToISOInitialEntry(mapFile, pcVE)) &&
			(CheckISOIsBootable(pcIE) == ISO_PROCESS_SUCCESS))			/*!< 检测是否可以启动 */
		{
			const void *entryPtr = JumpToISOBootableImage(mapFile, pcIE);

			entry->pvFile = (void *)(entryPtr);
			entry->size = mapFile->size - ((char *)entryPtr - (char *)mapFile->pvFile);
		}

		/*检测跳转后的entry是否有效*/
		if(CHECK_FILE_PTR_IS_VALID(entry))
		{
			/*尝试从ISO的BootImage入口跳转到IMG入口*/
			if(CheckIMGIdentifier(entry) != IMAGE_PROCESS_SUCCESS)
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
				(CheckIMGIdentifier(entry) == IMAGE_PROCESS_SUCCESS) &&	/*!< 文件标识有效 */
				(CheckIMGFileSystem(entry) == IMAGE_PROCESS_SUCCESS) &&	/*!< 文件系统检查 */
				(CHECK_PTR_SPACE_IS_VALID(entry, entry->pvFile, 512)))		/*!< BPB可读检查 */
				retVal = PROCESS_SUCCESS;//找到IMG入口
		}

	}

	return retVal;
}

MEDIA_TYPE GetInputType(const File *mapFile)
{
	MEDIA_TYPE retVal = UNKNOWN;

	if(CHECK_FILE_PTR_IS_VALID(mapFile))
	{
		//检查是否为IMG
		if(CheckIMGIdentifier(mapFile) == IMAGE_PROCESS_SUCCESS)
			retVal = IMG;//是IMG格式
		else
		{
			const PrimVolDesc *pcPVD = JumpToISOPrimVolDesc(mapFile);

			if(pcPVD && CheckISOIdentifier(pcPVD) == ISO_PROCESS_SUCCESS)
				retVal = ISO;//是ISO格式
		}
	}
	
	return retVal;
}

int DumpIMGFromISO(const File *mapFile, const char *isoPath)
{
	int retVal = PROCESS_FAILED;

	File entry;
	const char *imgPath = GetOutPath(isoPath, ".img");

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && imgPath)
	{
		ColorPrintf(WHITE, "尝试从ISO文件中提取IMG:\n\n");
		ColorPrintf(WHITE, "%s正在寻找IMG入口\t\t\t\t", leftMargin);

		if(GetBootEntryFromISO(mapFile, &entry) == PROCESS_SUCCESS)
		{
			ColorPrintf(LIME, "成功\n");
			ColorPrintf(WHITE, "%s正在写入IMG文件\t\t\t\t", leftMargin);

			if(WriteIMGToFile(&entry, imgPath) == PROCESS_SUCCESS)
			{
				ColorPrintf(LIME, "成功\n");
				ColorPrintf(WHITE, "\n处理完毕\n");

				retVal = PROCESS_SUCCESS;//写入成功！
			}
			else
				ColorPrintf(RED, "失败\n");
		}
		else
			ColorPrintf(RED, "失败\n");
	}

	return retVal;
}

int DisplayISOInfo(const File *mapFile)
{
	int retVal = PROCESS_FAILED;

	if(CHECK_FILE_PTR_IS_VALID(mapFile))
	{
		const PrimVolDesc *pcPVD = NULL;
		const BootRecordVolDesc *pcBRVD = NULL;
		const ValidationEntry *pcVE = NULL;
		const InitialEntry *pcIE = NULL;

		ColorPrintf(WHITE, "检测到ISO文件信息如下：\n\n");

		/*! 输出PrimVolDesc中信息 */
		if((pcPVD = JumpToISOPrimVolDesc(mapFile)))
		{
			const ISO9660TimeStr *createDate = (const ISO9660TimeStr *)(pcPVD->VolCreationDate);
			const ISO9660TimeStr *modifyDate = (const ISO9660TimeStr *)(pcPVD->VolModifDate);

			uint32_t volBlocks = LD_UINT32(pcPVD->VolSpaceSz);
			uint16_t bytsPerBlocks = LD_UINT16(pcPVD->LogicalBlockSz);

			ColorPrintf(WHITE, "%s光盘标签：\t\t\t\t", leftMargin);
			ColorPrintf(LIME, "%.32s\n", pcPVD->VolID);
			ColorPrintf(YELLOW, "%s卷逻辑块数：\t\t\t", leftMargin);
			ColorPrintf(AQUA, "%u\n", volBlocks);
			ColorPrintf(YELLOW, "%s每逻辑块字节数：\t\t\t", leftMargin);
			ColorPrintf(AQUA, "%u\n", bytsPerBlocks);
			ColorPrintf(YELLOW, "%s光盘容量：\t\t\t\t", leftMargin);
			ColorPrintf(AQUA, "%u\n", volBlocks * bytsPerBlocks);

			if(createDate && modifyDate)
			{
				ColorPrintf(WHITE, "%s创建日期：\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%.4s/%.2s/%.2s %.2s:%.2s:%.2s\n",
					createDate->Year, createDate->Month, createDate->Day,
					createDate->Hour, createDate->Minute, createDate->Second);

				ColorPrintf(WHITE, "%s修改日期：\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%.4s/%.2s/%.2s %.2s:%.2s:%.2s\n",
					createDate->Year, createDate->Month, createDate->Day,
					createDate->Hour, createDate->Minute, createDate->Second);
			}
		}

		/*! 输出BootRecordVolDesc中信息 */
		if((pcBRVD = JumpToISOBootRecordVolDesc(mapFile, pcPVD)) &&
			(CheckISOBootIdentifier(pcBRVD) == ISO_PROCESS_SUCCESS))
		{
			ColorPrintf(YELLOW, "%s启动规范：\t\t\t\t", leftMargin);
			ColorPrintf(AQUA, "EL TORITO\n");

			/*! 输出ValidationEntry中信息 */
			if((pcVE = JumpToISOValidationEntry(mapFile, pcBRVD)))
			{
				ColorPrintf(WHITE, "%s支持平台：\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%s\n", GetISOPlatformID(pcVE));
			}

			/*! 输出InitialEntry中信息 */
			if((pcIE = JumpToISOInitialEntry(mapFile, pcVE)))
			{
				ColorPrintf(WHITE, "%s光盘类型：\t\t\t\t", leftMargin);
				if(CheckISOIsBootable(pcIE) == ISO_PROCESS_SUCCESS)
					ColorPrintf(LIME, "可启动光盘\n");
				else
					ColorPrintf(LIME, "非启动光盘\n");

				ColorPrintf(YELLOW, "%s启动介质类型：\t\t\t", leftMargin);
				ColorPrintf(AQUA, "%s\n", GetISOBootMediaType(pcIE));
			}
		}

		printf("\n");

		retVal = PROCESS_SUCCESS;
	}
	return retVal;
}

int DisplayIMGInfo(const File *mapFile)
{
	static const char *FileSysType[] = 
	{
		"FAT12",
		"FAT16",
		"FAT32"
	};

	int retVal = PROCESS_FAILED;
	
	if(CHECK_FILE_PTR_IS_VALID(mapFile))
	{
		const BPB *pcBPB = (const BPB *)(mapFile->pvFile);
		const char *fsType = NULL;
		uint32_t totalSec, numTrks, volID = 0, secPerFat = 0;
		uint8_t drvNum = 0;

		if(LD_UINT16(pcBPB->Common.BPB_TotSec16))
			totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
		else
			totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

		numTrks = totalSec / 
			(LD_UINT16(pcBPB->Common.BPB_NumHeads) * LD_UINT16(pcBPB->Common.BPB_SecPerTrk));

		switch(GetIMGType(pcBPB))
		{
		case FAT12:
			volID = LD_UINT32(pcBPB->Special.Fat16.BS_VolID);
			drvNum = LD_UINT8(pcBPB->Special.Fat16.BS_DrvNum);
			secPerFat = LD_UINT16(pcBPB->Common.BPB_FATSz16);
			fsType = FileSysType[0];
			break;
		case FAT16:
			volID = LD_UINT32(pcBPB->Special.Fat16.BS_VolID);
			drvNum = LD_UINT8(pcBPB->Special.Fat16.BS_DrvNum);
			secPerFat = LD_UINT16(pcBPB->Common.BPB_FATSz16);
			fsType = FileSysType[1];
			break;
		case FAT32:
			volID = LD_UINT32(pcBPB->Special.Fat32.BS_VolID);
			drvNum = LD_UINT8(pcBPB->Special.Fat32.BS_DrvNum);
			secPerFat = LD_UINT32(pcBPB->Special.Fat32.BPB_FATSz32);
			fsType = FileSysType[2];
			break;
		}

		ColorPrintf(WHITE, "%s系统标名称：\t\t\t", leftMargin);
		ColorPrintf(LIME, "%.8s\n", pcBPB->Common.BS_OEMName);

		ColorPrintf(WHITE, "%s卷序列号：\t\t\t\t", leftMargin);
		ColorPrintf(LIME, "0x%.4X\n", volID);

		ColorPrintf(WHITE, "%s介质类型：\t\t\t\t", leftMargin);
		ColorPrintf(LIME, "0x%.1X\n", LD_UINT8(pcBPB->Common.BPB_Media));

		ColorPrintf(WHITE, "%s物理驱动器号：\t\t\t", leftMargin);
		ColorPrintf(LIME, "0x%.2X\n", drvNum);

		ColorPrintf(WHITE, "%s文件系统：\t\t\t\t", leftMargin);
		ColorPrintf(LIME, "%s\n", fsType);

		ColorPrintf(WHITE, "%s每扇区字节数：\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT16(pcBPB->Common.BPB_BytsPerSec));

		ColorPrintf(WHITE, "%sFAT表数：\t\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT8(pcBPB->Common.BPB_NumFATs));

		ColorPrintf(WHITE, "%s每FAT表扇区数：\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", secPerFat);

		ColorPrintf(WHITE, "%s隐藏扇区数：\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT32(pcBPB->Common.BPB_HiddSec));
		
		ColorPrintf(WHITE, "%s每簇扇区数：\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT8(pcBPB->Common.BPB_SecPerClus));

		ColorPrintf(WHITE, "%s保留扇区数：\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT16(pcBPB->Common.BPB_RsvdSecCnt));

		ColorPrintf(WHITE, "%s根目录项数：\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT16(pcBPB->Common.BPB_RootEntCnt));

		ColorPrintf(YELLOW, "%s磁道数（C）：\t\t\t", leftMargin);
		ColorPrintf(AQUA, "%u\n", numTrks);

		ColorPrintf(YELLOW, "%s磁头数（H）：\t\t\t", leftMargin);
		ColorPrintf(AQUA, "%u\n", LD_UINT16(pcBPB->Common.BPB_NumHeads));

		ColorPrintf(YELLOW, "%s每磁道扇区数（S）：\t\t\t", leftMargin);
		ColorPrintf(AQUA, "%u\n", LD_UINT16(pcBPB->Common.BPB_SecPerTrk));

		ColorPrintf(FUCHSIA, "%s扇区总数：\t\t\t\t", leftMargin);
		ColorPrintf(AQUA, "%u\n", totalSec);
	}

	return retVal;
}

//========================测试代码开始=============================

#ifdef _DEBUG

void TestISO(const File *mapFile)
{
	ISOTestUnit(mapFile);
}

void TestIMG(const File *mapFile)
{
	IMGTestUnit(mapFile);
}

#endif

//========================测试代码结束=============================
