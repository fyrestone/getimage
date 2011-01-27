/*!
\file IMGProcess.h
\author LiuBao
\version 1.0
\date 2010/12/25
\brief 声明IMG处理函数，定义FAT文件系统的结构体
*/
#ifndef IMAGE_PROCESS
#define IMAGE_PROCESS

#include "MapFile.h"

#define MIN_FAT16 4085	///< FAT16数据最少簇数	
#define MIN_FAT32 65525	///< FAT32数据最少簇数	

typedef int FS_TYPE;	///< FAT格式定义

#define FAT_ERR 0x10	///< 错误FAT格式	
#define FAT12 0x11		///< FAT12格式
#define FAT16 0x12		///< FAT16格式
#define FAT32 0x13		///< FAT32格式		

/*
	BPB->BIOS Parameter Block
	BS->Boot Sector
*/

/*
	由于存在内存对齐问题，这里又使用了结构体，因此，必须在编译时确认结构体的大小，
	以保证数据正确对齐。在对移植性要求较高情况下应当使用宏定义相对偏移量来代替结构体！
*/

typedef struct _BPBCommon		///  BPB中FAT12/16/32共用部分
{
	uint8_t BS_JmpBoot[3];		///< 跳转指令，跳转到DBR中的引导程序
	uint8_t BS_OEMName[8];		///< 卷的OEM名称
	uint8_t BPB_BytsPerSec[2];	///< 每扇区包含字节数
	uint8_t BPB_SecPerClus[1];	///< 每簇包含扇区数
	uint8_t BPB_RsvdSecCnt[2];	///< 保留扇区数目，指第一个FAT表开始前的扇区数，包括DBR本身
	uint8_t BPB_NumFATs[1];		///< 卷FAT表数
	uint8_t BPB_RootEntCnt[2];	///< 卷根入口点数
	uint8_t BPB_TotSec16[2];	///< 卷扇区总数，对于大于65535个扇区的卷，本字段为0
	uint8_t BPB_Media[1];		///< 卷介质类型
	uint8_t BPB_FATSz16[2];		///< 每个FAT表占用扇区数
	uint8_t BPB_SecPerTrk[2];	///< 每磁道包含扇区数
	uint8_t BPB_NumHeads[2];	///< 卷磁头数
	uint8_t BPB_HiddSec[4];		///< 卷隐藏磁头数
	uint8_t BPB_TotSec32[4];	///< 卷扇区总数，大于65535个扇区的卷用本字段表示
}BPBCommon;

typedef struct _BPBFat16		///  BPB中FAT16特殊部分
{
	uint8_t BS_DrvNum[1];		///< 驱动器编号
	uint8_t BS_Reserved1[1];	///< 保留字段
	uint8_t BS_BootSig[1];		///< 磁盘扩展引导区标签，Windows要求该标签为0x28或者0x29
	uint8_t BS_VolID[4];		///< 磁盘卷ID
	uint8_t BS_VolLab[11];		///< 磁盘卷标
	uint8_t BS_FilSysType[8];	///< 磁盘上的文件系统类型
}BPBFat16;

typedef struct _BPBFat32		///  BPB中FAT32特殊部分
{
	uint8_t BPB_FATSz32[4];		///< 每个FAT表占用扇区数，FAT32特有，此时BPB_FATSz16必须为0
	uint8_t BPB_ExtFlags[2];	///< [0:3]活动FAT表索引号\n [4:6]保留\n [7]为0镜像到所有FAT表，为1只有一个活动FAT表\n [8:15]保留
	uint8_t BPB_FSVer[2];		///< 高位为FAT32的主版本号，低位为次版本号
	uint8_t BPB_RootClus[4];	///< 根目录所在第一个簇的簇号，通常该数值为2，但不是必须为2
	uint8_t BPB_FSInfo[2];		///< 保留区中FAT32卷FSINFO结构所占的扇区数，通常为1
	uint8_t BPB_BkBootSec[2];	///< 如果不为0，表示在保留区中引导记录的备份数据所占的扇区数，通常为6
	uint8_t BPB_Reserved[12];	///< 保留
	uint8_t BS_DrvNum[1];		///< 驱动器编号
	uint8_t BS_Reserved1[1];	///< 保留
	uint8_t BS_BootSig[1];		///< 磁盘扩展引导区标签，Windows要求该标签为0x28或者0x29
	uint8_t BS_VolID[4];		///< 磁盘卷ID
	uint8_t BS_VolLab[11];		///< 磁盘卷标
	uint8_t BS_FilSysType[8];	///< 磁盘上的文件系统类型
}BPBFat32;

typedef struct _BPB				///  512比特IMG头部
{
	BPBCommon Common;			///< BPB中FAT12/16/32共用部分
	union
	{
		BPBFat16 Fat16;
		BPBFat32 Fat32;
	}Special;					///< BPB中FAT12/16与FAT32不同部分
	uint8_t Interval[420];		///< 间隔
	uint8_t EndFlag[2];			///< DBR结束签名
}BPB;

/*!
检查IMG文件的识别标记（跳转标记）
\param media 位置在IMG头部的media_t
\return 如果检测通过（是IMG）返回SUCCESS；否则返回FAILED
*/
int CheckIMGIdentifier(media_t media);

/*!
检查IMG文件系统识别标记
\param media 位置在IMG头部的media_t
\return 如果检测通过（是IMG文件系统）返回SUCCESS；否则返回FAILED
*/
int CheckIMGFileSystem(media_t media);

/*!
获取IMG文件系统类型
\param pcBPB BPB结构体指针
\return FAT12/FAT16/FAT32/FAT_ERR
*/
FS_TYPE GetIMGType(const BPB *pcBPB);

#endif