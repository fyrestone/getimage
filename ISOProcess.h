#ifndef ISO_PROCESS
#define ISO_PROCESS

#include "MapFile.h"

#define ISO_PROCESS_SUCCESS 0
#define ISO_PROCESS_FAILED -1

#define SECTOR_SIZE 2048			/*!< ISO每扇区比特数 */

/*!
	由于存在内存对齐问题，这里又使用了结构体，因此，必须在编译时确认结构体的大小，
	以保证数据正确对齐。在对移植性要求较高情况下应当使用宏定义相对偏移量来代替结构体！
*/

typedef struct _PrimVolDesc
{
	uint8_t PrimaryIndicator;		/*!< 必为1 */
	uint8_t ISO9660Identifier[5];	/*!< 必为CD001 */
	uint8_t DescVer;				/*!< Description Version，必为1 */
	uint8_t Unused_1;				/*!< 应为00 */
	uint8_t SysID[32];				/*!< System Identifier */
	uint8_t VolID[32];				/*!< Volumn Identifier */
	uint8_t Unused_2[8];			/*!< 应为00 */
	uint8_t VolSpaceSz[8];			/*!< Volumn Space Size,733标准 */
	uint8_t Unused_3[32];			/*!< 应为00 */
	uint8_t VolSetSz[4];			/*!< Volumn Set Size */
	uint8_t VolSeqNum[4];			/*!< Volumn Sequence Number */
	uint8_t LogicalBlockSz[4];		/*!< Logical Block Size,723标准 */
	uint8_t PathTableSz[8];			/*!< Path Table Size */
	uint8_t TypeIPathTable[4];		/*!< Location of Occurrence of Type L Path Table */
	uint8_t OptTypeIPathTable[4];	/*!< Location of Optional Occurrence of Type L Path Table */
	uint8_t TypeMPathTable[4];		/*!< Location of Occurrence of Type M Path Table */
	uint8_t OptTypeMPathTable[4];	/*!< Location of Optional Occurrence of Type M Path Table */
	uint8_t RootDirRecord[34];		/*!< Root Directory Record */
	uint8_t VolSetID[128];			/*!< Volumn Set Identifier */
	uint8_t PublID[128];			/*!< Publisher Identifier */
	uint8_t DataPrepID[128];		/*!< Data Preparer Identifier */
	uint8_t AppID[128];				/*!< Application Identifier */
	uint8_t CopFileID[37];			/*!< Copyright File Identifier */
	uint8_t AbsFileID[37];			/*!< Abstract File Identifier */
	uint8_t BibliogFileID[37];		/*!< Bibliographic File Identifier */
	uint8_t VolCreationDate[17];	/*!< Volumn Creation Date And Time */
	uint8_t VolModifDate[17];		/*!< Volumn Modification Date And Time */
	uint8_t VolExpDate[17];			/*!< Volumn Expiration Date And Time */
	uint8_t VolEffectiveDate[17];	/*!< Volumn Effective Date And Time */
	uint8_t FileStructureVer;		/*!< File Structure Version */
	uint8_t Reserved_1;				/*!< 应为00 */
	uint8_t AppUse[512];			/*!< Application Use */
	uint8_t Reserved_2[653];
}PrimVolDesc;

typedef struct _BootRecordVolDesc
{
	uint8_t BootRecordIndic;		/*!< Boot Record Indicator，必为0 */
	uint8_t ISO9660ID[5];			/*!< ISO9660 Identifier，必为CD001 */
	uint8_t DescVer;				/*!< Description Version，必为1 */
	uint8_t BootSysID[32];			/*!< Boot System Identifier，符合EL TORITO启动规范应以“EL TORITO SPECIFICATION”开头 */
	uint8_t Unused_1[32];
	uint8_t SecToBootCat[4];		/*!< Sectors To Boot Catalog，DWORD，跳转到BootCatalog第一个扇区所需扇区数 */
	uint8_t Unused_2[1973];
}BootRecordVolDesc;

typedef struct _ValidationEntry
{
	uint8_t HeaderID;				/*!< 必为01 */
	uint8_t PlatformID;				/*!< 0=80x86；1=Power PC；2=Mac */
	uint8_t Reserved[2];
	uint8_t ManufacturerID[24];		/*!< 生产商标识 */
	uint8_t CheckSum[2];			/*!< WORD，应当为0 */
	uint8_t EndofVE[2];				/*!< 必为55AA */
}ValidationEntry;

typedef struct _InitialEntry
{
	uint8_t BootIndicator;			/*!< 88=Bootable；00=Not Bootable */
	/*! 
		0-3位含义如下，4-7位保留并且必为0↓
		0 No Emulation
		1 1.2 meg diskette
		2 1.44 meg diskette
		3 2.88 meg diskette
		4 Hard Disk(drive 80)
	 */
	uint8_t BootMediaType;
	uint8_t LoadSegment[2];			/*!< WORD，仅对x86架构有效，如果该值为0，使用默认段7C0，否则使用指定段 */
	uint8_t SystemType;
	uint8_t Unused_1;
	uint8_t SectorCount[2];			/*!< 启动程序加载的扇区数 */
	uint8_t LoadRBA[4];				/*!< DWORD，Virtual Disk的开始地址 */
	uint8_t Unused_2[20];
}InitialEntry;

typedef struct _ISO9660TimeStr{
  uint8_t Year[4];
  uint8_t Month[2];					/*!< 月份：1...12 */
  uint8_t Day[2];					/*!< 日期：1...31 */
  uint8_t Hour[2];					/*!< 小时：0...23 */
  uint8_t Minute[2];				/*!< 分钟：0...59 */
  uint8_t Second[2];				/*!< 秒：0...59 */
  uint8_t CentiSecond[2];			/*!< 1/100秒 */
  int8_t GMTOffset;					/*!< 偏离格林威治时间的值，以15分钟间隔为单位，从-48（东部）到+52（西部） */
}ISO9660TimeStr;

const PrimVolDesc *JumpToISOPrimVolDesc(const File *mapFile);

const BootRecordVolDesc *JumpToISOBootRecordVolDesc(const File *mapFile, const PrimVolDesc *pcPVD);

const ValidationEntry *JumpToISOValidationEntry(const File *mapFile, const BootRecordVolDesc *pcBRVD);

const InitialEntry *JumpToISOInitialEntry(const File *mapFile, const ValidationEntry *pcVE);

const void *JumpToISOBootableImage(const File *mapFile, const InitialEntry *pcIE);

int CheckISOIdentifier(const PrimVolDesc *pcPVD);

int CheckISOBootIdentifier(const BootRecordVolDesc *pcBRVD);

int CheckISOIsBootable(const InitialEntry *pcIE);

const char *GetISOPlatformID(const ValidationEntry *pcVE);

const char *GetISOBootMediaType(const InitialEntry *pcIE);

#ifdef _DEBUG
	void ISOTestUnit(const File *mapFile);
#endif

#endif