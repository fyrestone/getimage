#ifndef IMAGE_PROCESS
#define IMAGE_PROCESS

#include "MapFile.h"

#define IMAGE_PROCESS_SUCCESS 0
#define IMAGE_PROCESS_FAILED -1

/*! 用于检测磁盘格式 */
#define MIN_FAT16 4085			/*!< FAT16数据最少簇数 */
#define MIN_FAT32 65525			/*!< FAT32数据最少簇数 */

/*! FAT格式定义 */
typedef int FS_TYPE;

#define FAT_ERR 0x10			/*!< 错误FAT格式 */
#define FAT12 0x11
#define FAT16 0x12
#define FAT32 0x13

/*!
	BPB->BIOS Parameter Block
	BS->Boot Sector
*/

/*!
	由于存在内存对齐问题，这里又使用了结构体，因此，必须在编译时确认结构体的大小，
	以保证数据正确对齐。在对移植性要求较高情况下应当使用宏定义相对偏移量来代替结构体！
*/

typedef struct _BPBCommon
{
	uint8_t BS_JmpBoot[3];		/*!< 跳转指令，跳转到DBR中的引导程序 */
	uint8_t BS_OEMName[8];		/*!< 卷的OEM名称 */
	uint8_t BPB_BytsPerSec[2];	/*!< 每扇区包含字节数 */
	uint8_t BPB_SecPerClus[1];	/*!< 每簇包含扇区数 */
	uint8_t BPB_RsvdSecCnt[2];	/*!< 保留扇区数目，指第一个FAT表开始前的扇区数，包括DBR本身 */
	uint8_t BPB_NumFATs[1];		/*!< 卷FAT表数 */
	uint8_t BPB_RootEntCnt[2];	/*!< 卷根入口点数 */
	uint8_t BPB_TotSec16[2];	/*!< 卷扇区总数，对于大于65535个扇区的卷，本字段为0 */
	uint8_t BPB_Media[1];		/*!< 卷介质类型 */
	uint8_t BPB_FATSz16[2];		/*!< 每个FAT表占用扇区数 */
	uint8_t BPB_SecPerTrk[2];	/*!< 每磁道包含扇区数 */
	uint8_t BPB_NumHeads[2];	/*!< 卷磁头数 */
	uint8_t BPB_HiddSec[4];		/*!< 卷隐藏磁头数 */
	uint8_t BPB_TotSec32[4];	/*!< 卷扇区总数，大于65535个扇区的卷用本字段表示 */
}BPBCommon;//=>Fat12/16/32共用部分。

typedef struct _BPBFat16
{
	uint8_t BS_DrvNum[1];		/*!< 驱动器编号 */
	uint8_t BS_Reserved1[1];	/*!< 保留字段 */
	uint8_t BS_BootSig[1];		/*!< 磁盘扩展引导区标签，Windows要求该标签为0x28或者0x29 */
	uint8_t BS_VolID[4];		/*!< 磁盘卷ID */
	uint8_t BS_VolLab[11];		/*!< 磁盘卷标 */
	uint8_t BS_FilSysType[8];	/*!< 磁盘上的文件系统类型 */
}BPBFat16;//=>Fat16特殊部分。

typedef struct _BPBFat32
{
	uint8_t BPB_FATSz32[4];
	uint8_t BPB_ExtFlags[2];
	uint8_t BPB_FSVer[2];
	uint8_t BPB_RootClus[4];
	uint8_t BPB_FSInfo[2];
	uint8_t BPB_BkBootSec[2];
	uint8_t BPB_Reserved[12];
	uint8_t BS_DrvNum[1];
	uint8_t BS_Reserved1[1];
	uint8_t BS_BootSig[1];
	uint8_t BS_VolID[4];
	uint8_t BS_VolLab[11];
	uint8_t BS_FilSysType[8];
}BPBFat32;//=>Fat32特殊部分。

typedef struct _BPB
{
	BPBCommon Common;
	union
	{
		BPBFat16 Fat16;
		BPBFat32 Fat32;
	}Special;
	uint8_t Interval[420];		/*!< 应为Fat32格式下启动程序 */
	uint8_t EndFlag[2];			/*!< DBR结束签名 */
}BPB;//=>512比特IMG头部。

int CheckIMGIdentifier(const File *mapFile);

int CheckIMGFileSystem(const File *mapFile);

FS_TYPE GetIMGType(const BPB *pcBPB);

void IMGTestUnit(const File *mapFile);

#endif