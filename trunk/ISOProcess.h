#ifndef ISO_PROCESS
#define ISO_PROCESS

#include "MapFile.h"

#define ISO_PROCESS_SUCCESS 0
#define ISO_PROCESS_FAILED -1

#define SECTOR_SIZE 2048			/*!< ISOÿ���������� */

/*!
	���ڴ����ڴ�������⣬������ʹ���˽ṹ�壬��ˣ������ڱ���ʱȷ�Ͻṹ��Ĵ�С��
	�Ա�֤������ȷ���롣�ڶ���ֲ��Ҫ��ϸ������Ӧ��ʹ�ú궨�����ƫ����������ṹ�壡
*/

typedef struct _PrimVolDesc
{
	uint8_t PrimaryIndicator;		/*!< ��Ϊ1 */
	uint8_t ISO9660Identifier[5];	/*!< ��ΪCD001 */
	uint8_t DescVer;				/*!< Description Version����Ϊ1 */
	uint8_t Unused_1;				/*!< ӦΪ00 */
	uint8_t SysID[32];				/*!< System Identifier */
	uint8_t VolID[32];				/*!< Volumn Identifier */
	uint8_t Unused_2[8];			/*!< ӦΪ00 */
	uint8_t VolSpaceSz[8];			/*!< Volumn Space Size */
	uint8_t Unused_3[32];			/*!< ӦΪ00 */
	uint8_t VolSetSz[4];			/*!< Volumn Set Size */
	uint8_t VolSeqNum[4];			/*!< Volumn Sequence Number */
	uint8_t LogicalBlockSz[4];		/*!< Logical Block Size */
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
	uint8_t Reserved_1;				/*!< ӦΪ00 */
	uint8_t AppUse[512];			/*!< Application Use */
	uint8_t Reserved_2[653];
}PrimVolDesc;

typedef struct _BootRecordVolDesc
{
	uint8_t BootRecordIndic;		/*!< Boot Record Indicator����Ϊ0 */
	uint8_t ISO9660ID[5];			/*!< ISO9660 Identifier����ΪCD001 */
	uint8_t DescVer;				/*!< Description Version����Ϊ1 */
	uint8_t BootSysID[32];			/*!< Boot System Identifier������EL TORITO�����淶Ӧ�ԡ�EL TORITO SPECIFICATION����ͷ */
	uint8_t Unused_1[32];
	uint8_t SecToBootCat[4];		/*!< Sectors To Boot Catalog��DWORD����ת��BootCatalog��һ���������������� */
	uint8_t Unused_2[1973];
}BootRecordVolDesc;

typedef struct _ValidationEntry
{
	uint8_t HeaderID;				/*!< ��Ϊ01 */
	uint8_t PlatformID;				/*!< 0=80x86��1=Power PC��2=Mac */
	uint8_t Reserved[2];
	uint8_t ManufacturerID[24];		/*!< �����̱�ʶ */
	uint8_t CheckSum[2];			/*!< WORD��Ӧ��Ϊ0 */
	uint8_t EndofVE[2];				/*!< ��Ϊ55AA */
}ValidationEntry;

typedef struct _InitialEntry
{
	uint8_t BootIndicator;			/*!< 88=Bootable��00=Not Bootable */
	/*! 
		0-3λ�������£�4-7λ�������ұ�Ϊ0��
		0 No Emulation
		1 1.2 meg diskette
		2 1.44 meg diskette
		3 2.88 meg diskette
		4 Hard Disk(drive 80)
	 */
	uint8_t BootMediaType;
	uint8_t LoadSegment[2];			/*!< WORD������x86�ܹ���Ч�������ֵΪ0��ʹ��Ĭ�϶�7C0������ʹ��ָ���� */
	uint8_t SystemType;
	uint8_t Unused_1;
	uint8_t SectorCount[2];			/*!< ����������ص������� */
	uint8_t LoadRBA[4];				/*!< DWORD��Virtual Disk�Ŀ�ʼ��ַ */
	uint8_t Unused_2[20];
}InitialEntry;

typedef struct _ISO9660TimeStr{
  uint8_t Year[4];
  uint8_t Month[2];					/*!< �·ݣ�1...12 */
  uint8_t Day[2];					/*!< ���ڣ�1...31 */
  uint8_t Hour[2];					/*!< Сʱ��0...23 */
  uint8_t Minute[2];				/*!< ���ӣ�0...59 */
  uint8_t Second[2];				/*!< �룺0...59 */
  uint8_t CentiSecond[2];			/*!< 1/100�� */
  int8_t GMTOffset;					/*!< ƫ���������ʱ���ֵ����15���Ӽ��Ϊ��λ����-48����������+52�������� */
}ISO9660TimeStr;

const PrimVolDesc *JumpToPrimVolDesc(const File *mapFile);

const BootRecordVolDesc *JumpToBootRecordVolDesc(const File *mapFile, const PrimVolDesc *pcPVD);

const ValidationEntry *JumpToValidationEntry(const File *mapFile, const BootRecordVolDesc *pcBRVD);

const InitialEntry *JumpToInitialEntry(const File *mapFile, const ValidationEntry *pcVE);

const void *JumpToBootableImage(const File *mapFile, const InitialEntry *pcIE);

int CheckISOIdentifier(const PrimVolDesc *pcPVD);

int CheckBootIdentifier(const BootRecordVolDesc *pcBRVD);

const char *CheckBootIndicator(const InitialEntry *pcIE);

const char *CheckBootMediaType(const InitialEntry *pcIE);

void ISOTestUnit(const File *mapFile);

#endif