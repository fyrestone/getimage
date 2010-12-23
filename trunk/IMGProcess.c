#include <assert.h>
#include "IMGProcess.h"

int CheckIMGIdentifier(media_t media)
{
	int retVal = IMAGE_PROCESS_FAILED;

	media_access access;
	if(GetMediaAccess(media, &access, 3) == MAP_SUCCESS)
	{
		//EB??90 || E9????
		if((access.begin[0] == 0xEB && access.begin[2] == 0x90) ||
			access.begin[0] == 0xE9)
			retVal = IMAGE_PROCESS_SUCCESS;
	}

	return retVal;
}

int CheckIMGFileSystem(media_t media)
{
	media_access access;

	if(GetMediaAccess(media, &access, 512) == MAP_SUCCESS)
	{
		/*若0x55AA结尾，认为其为BR*/
		if(LD_UINT16(access.begin + 510) == (uint16_t)0xAA55)
		{
			const BPB *pcBPB = (const BPB *)access.begin;

			/*检测“FAT”字符串*/
			if((LD_UINT32(pcBPB->Special.Fat16.BS_FilSysType) & 0xFFFFFF) == 0x544146)
				return IMAGE_PROCESS_SUCCESS;//检测成功

			if((LD_UINT32(pcBPB->Special.Fat32.BS_FilSysType) & 0xFFFFFF) == 0x544146)
				return IMAGE_PROCESS_SUCCESS;//检测成功
		}
	}

	return IMAGE_PROCESS_FAILED;//检测失败
}

FS_TYPE GetIMGType(const BPB *pcBPB)
{
	FS_TYPE retVal = FAT_ERR;

	/*!
		FAT的总扇区数（TotalSec）由卷保留扇区数（RsvdSecCnt）、FAT表所占扇区数（NumFats * FatSz）、
		根目录项所占扇区数（RootDirSec，由于FAT32的根目录项等同于普通文件，FAT32的RootDirSectors为0）、
		数据所占扇区数（DataSec）四部分依次构成。其中保留扇区数包含了FAT头部（BPB）
	*/
	if(pcBPB)
	{
		uint32_t totalSec;		/*!< 总扇区数 */
		uint32_t rsvdSec;		/*!< 保留扇区数（包含FAT头的512字节） */
		uint32_t fatSec;		/*!< FAT文件分配表所占扇区数 */
		uint32_t rootDirSec;	/*!< 根目录所占扇区数 */
		uint32_t dataSec;		/*!< 数据所占扇区数（包括不足一簇的扇区） */
		uint32_t dataClusters;	/*!< 数据所占簇数 */

		/* 计算totalSec */
		if(LD_UINT16(pcBPB->Common.BPB_TotSec16))
			totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
		else
			totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

		/* 计算rsvdSec */
		rsvdSec = LD_UINT16(pcBPB->Common.BPB_RsvdSecCnt);

		/* 计算fatSec */
		if(LD_UINT16(pcBPB->Common.BPB_FATSz16))
			fatSec = LD_UINT8(pcBPB->Common.BPB_NumFATs) * LD_UINT16(pcBPB->Common.BPB_FATSz16);
		else
			fatSec = LD_UINT8(pcBPB->Common.BPB_NumFATs) * LD_UINT32(pcBPB->Special.Fat32.BPB_FATSz32);

		/* 计算rootDirSec */
		rootDirSec = ((LD_UINT16(pcBPB->Common.BPB_RootEntCnt) * 32) + (LD_UINT16(pcBPB->Common.BPB_BytsPerSec) - 1))
			/ LD_UINT16(pcBPB->Common.BPB_BytsPerSec);

		/* 计算dataSec */
		dataSec = totalSec - rsvdSec - fatSec - rootDirSec;

		/* 计算dataClusters */
		dataClusters = dataSec / LD_UINT8(pcBPB->Common.BPB_SecPerClus);

		assert(dataSec);

		retVal = FAT12;
		if(dataClusters >= MIN_FAT16)
			retVal = FAT16;
		if(dataClusters >= MIN_FAT32)
			retVal = FAT32;
	}

	return retVal;
}

//========================测试代码开始=============================

#ifdef _DEBUG
#include "ColorPrint.h"

void IMGTestUnit(media_t media)
{
#define TEST_IF_TRUE_SET_ERR_AND_BREAK(condition, errStr)	\
	if(condition)											\
	{														\
		errStrPtr = #errStr;								\
		break;												\
	}																

	const char *errStrPtr = (const char *)0;

	ColorPrintf(AQUA, "运行IMG测试：\n");

	do
	{
		ColorPrintf(RED, "无测试用例！\n");
	}while(0);

	if(errStrPtr)//测试有错误发生
	{
		ColorPrintf(RED, "%s\n", errStrPtr);
	}

#undef TEST_IF_TRUE_SET_ERR_AND_BREAK
}

#endif

//========================测试代码结束=============================

