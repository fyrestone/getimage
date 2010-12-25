#include <Windows.h>	/*!< 使用MAX_PATH宏 */
#include <stdio.h>		/*!< 使用FILE */
#include <assert.h>
#include "ProjDef.h"
#include "ColorPrint.h"
#include "Process.h"
#include "IMGProcess.h"
#include "ISOProcess.h"

static char PATH[MAX_PATH];						/*!< GetOutPath输出缓冲区，每次调用都会覆盖 */
static const char * const leftMargin = "    ";

/*!
获得输出路径，由iso路径获得提取出的img的写入路径（不可重入）
/param selfPath iso路径
/param extention 把iso扩展名修改为的扩展名，如'.img'
/return 输出路径
*/
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

/*!
从img起始位置提取出img写入到selfPath路径
\param imageEntry 位置在img起始的media_t
\param selfPath img写入路径
\return 成功返回SUCCESS；否则返回FAILED
*/
static int WriteIMGToFile(media_t imageEntry, const char *path)
{
	int retVal = FAILED;
	
	if(imageEntry && path)
	{
		media_access access;

		if(GetMediaAccess(imageEntry, &access, sizeof(BPB)) == SUCCESS)
		{
			const BPB *pcBPB = (const BPB *)access.begin;
			size_t totalSize = 0;//IMG大小

			/* 获得整个IMG大小 */
			{
				size_t totalSec = 0;

				if(LD_UINT16(pcBPB->Common.BPB_TotSec16))//如果是FAT12/16
					totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
				else//如果是FAT32
					totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

				totalSize = totalSec * LD_UINT16(pcBPB->Common.BPB_BytsPerSec);
			}

			/* 把IMG写入文件 */
			{
				FILE *outfp = fopen(path, "wb");

				if(DumpMedia(imageEntry, outfp, totalSize) == SUCCESS)
					retVal = SUCCESS;

				fclose(outfp);
			}
		}
	}

	return retVal;
}

/*!
从ISO文件头部跳转到启动IMG头部，内部自动处理Acronis启动光盘
\param media ISO文件的media_t
\return 成功返回SUCCESS；否则返回FAILED
*/
static int JumpToISOBootEntry(media_t media)
{
	int retVal = FAILED;

	if(media)
	{
		/*连续跳转指针*/
		if(JumpToISOPrimVolDesc(media) == SUCCESS &&
			JumpToISOBootRecordVolDesc(media) == SUCCESS &&
			JumpToISOValidationEntry(media) == SUCCESS &&
			JumpToISOInitialEntry(media) == SUCCESS &&
			JumpToISOBootableImage(media) == SUCCESS)
		{

			/*尝试从ISO的BootImage入口跳转到IMG入口*/
			if(CheckIMGIdentifier(media) != SUCCESS)
			{
				media_access access;
				if(GetMediaAccess(media, &access, 512) == SUCCESS)
				{
					/* Acronis光盘特殊跳转 */
					uint32_t offsetValue = LD_UINT8(access.begin + 244);
					uint32_t relativeOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
					
					(void)SeekMedia(media, relativeOffset, MEDIA_CUR);
				}
			}
		}

		/* 检查入口是否为FAT格式 */
		{
			media_access access;
			if(GetMediaAccess(media, &access, 512) == SUCCESS)
			{
				if(CheckIMGIdentifier(media) == SUCCESS &&
					CheckIMGFileSystem(media) == SUCCESS)
					retVal = SUCCESS;//找到IMG入口
			}
		}
	}

	return retVal;
}

MEDIA_TYPE GetInputType(media_t media)
{
	MEDIA_TYPE retVal = UNKNOWN;

	if(media)
	{
		if(CheckIMGIdentifier(media) == SUCCESS)
			retVal = IMG;//是IMG格式
		else if(JumpToISOPrimVolDesc(media) == SUCCESS)
		{
			(void)RewindMedia(media);
			retVal = ISO;//是ISO格式
		}
	}
	
	return retVal;
}

int DumpIMGFromISO(media_t media, const char *path)
{
	int retVal = FAILED;

	const char *imgPath = GetOutPath(path, ".img");

	if(media && imgPath)
	{
		ColorPrintf(WHITE, "尝试从ISO文件中提取IMG:\n\n");
		ColorPrintf(WHITE, "%s正在寻找IMG入口\t\t\t\t", leftMargin);

		do
		{
			/* 跳转到IMG头部 */
			if(JumpToISOBootEntry(media) == SUCCESS)
			{
				ColorPrintf(LIME, "成功\n");
				ColorPrintf(WHITE, "%s正在写入IMG文件\t\t\t\t", leftMargin);
			}
			else
			{
				ColorPrintf(RED, "失败\n");
				break;
			}

			/* 把IMG写入文件 */
			if(WriteIMGToFile(media, imgPath) == SUCCESS)
			{
				ColorPrintf(LIME, "成功\n");
				ColorPrintf(WHITE, "\n处理完毕\n");
			}
			else
			{
				ColorPrintf(RED, "失败\n");
				break;
			}

			retVal = SUCCESS;//写入成功！
		}while(0);
	}

	return retVal;
}

int DisplayISOInfo(media_t media)
{
	int retVal = FAILED;

	if(media)
	{
		media_access access;

		ColorPrintf(WHITE, "检测到ISO文件信息如下：\n\n");

		/*! 输出PrimVolDesc中信息 */
		if(JumpToISOPrimVolDesc(media) == SUCCESS)
		{
			if(GetMediaAccess(media, &access, sizeof(PrimVolDesc)) == SUCCESS)
			{
				const PrimVolDesc *pcPVD = (const PrimVolDesc *)access.begin;
				const ISO9660TimeStr *createDate = (const ISO9660TimeStr *)(pcPVD->VolCreationDate);
				const ISO9660TimeStr *modifyDate = (const ISO9660TimeStr *)(pcPVD->VolModifDate);

				uint32_t volBlocks = LD_UINT32(pcPVD->VolSpaceSz);//ISO卷逻辑块数
				uint16_t bytsPerBlocks = LD_UINT16(pcPVD->LogicalBlockSz);//逻辑块所占字节数

				ColorPrintf(WHITE, "%s光盘标签：\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%.32s\n", pcPVD->VolID);
				ColorPrintf(YELLOW, "%s卷逻辑块数：\t\t\t", leftMargin);
				ColorPrintf(AQUA, "%u\n", volBlocks);
				ColorPrintf(YELLOW, "%s每逻辑块字节数：\t\t\t", leftMargin);
				ColorPrintf(AQUA, "%u\n", bytsPerBlocks);
				ColorPrintf(YELLOW, "%s光盘容量：\t\t\t\t", leftMargin);
				ColorPrintf(AQUA, "%u\n", volBlocks * bytsPerBlocks);

				/* 输出创建日期和修改日期 */
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
		}

		/*! 输出BootRecordVolDesc中信息 */
		if(JumpToISOBootRecordVolDesc(media) == SUCCESS)
		{
			ColorPrintf(YELLOW, "%s启动规范：\t\t\t\t", leftMargin);
			ColorPrintf(AQUA, "EL TORITO\n");

			/*! 输出ValidationEntry中信息 */
			if(JumpToISOValidationEntry(media) == SUCCESS)
			{
				ColorPrintf(WHITE, "%s支持平台：\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%s\n", GetISOPlatformID(media));
			}

			/*! 输出InitialEntry中信息 */
			if(JumpToISOInitialEntry(media) == SUCCESS)
			{
				ColorPrintf(WHITE, "%s光盘类型：\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "可启动光盘\n");

				ColorPrintf(YELLOW, "%s启动介质类型：\t\t\t", leftMargin);
				ColorPrintf(AQUA, "%s\n", GetISOBootMediaType(media));
			}
		}

		printf("\n");

		retVal = SUCCESS;
	}
	return retVal;
}

int DisplayIMGInfo(media_t media)
{
	static const char *FileSysType[] = 
	{
		"FAT12",
		"FAT16",
		"FAT32"
	};

	int retVal = FAILED;

	media_access access;
	
	if(GetMediaAccess(media, &access, sizeof(BPB)) == SUCCESS)
	{
		const BPB *pcBPB = (const BPB *)access.begin;
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
