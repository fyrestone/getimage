#ifndef IMAGE_PROCESS
#define IMAGE_PROCESS

#include "MapFile.h"

#define IMAGE_PROCESS_SUCCESS 0
#define IMAGE_PROCESS_FAILED -1

/*! ���ڼ����̸�ʽ */
#define MIN_FAT16 4085			/*!< FAT16�������ٴ��� */
#define MIN_FAT32 65525			/*!< FAT32�������ٴ��� */

/*! FAT��ʽ���� */
typedef int FS_TYPE;

#define FAT_ERR 0x10			/*!< ����FAT��ʽ */
#define FAT12 0x11
#define FAT16 0x12
#define FAT32 0x13

/*!
	BPB->BIOS Parameter Block
	BS->Boot Sector
*/

/*!
	���ڴ����ڴ�������⣬������ʹ���˽ṹ�壬��ˣ������ڱ���ʱȷ�Ͻṹ��Ĵ�С��
	�Ա�֤������ȷ���롣�ڶ���ֲ��Ҫ��ϸ������Ӧ��ʹ�ú궨�����ƫ����������ṹ�壡
*/

typedef struct _BPBCommon
{
	uint8_t BS_JmpBoot[3];		/*!< ��תָ���ת��DBR�е��������� */
	uint8_t BS_OEMName[8];		/*!< ���OEM���� */
	uint8_t BPB_BytsPerSec[2];	/*!< ÿ���������ֽ��� */
	uint8_t BPB_SecPerClus[1];	/*!< ÿ�ذ��������� */
	uint8_t BPB_RsvdSecCnt[2];	/*!< ����������Ŀ��ָ��һ��FAT��ʼǰ��������������DBR���� */
	uint8_t BPB_NumFATs[1];		/*!< ��FAT���� */
	uint8_t BPB_RootEntCnt[2];	/*!< �����ڵ��� */
	uint8_t BPB_TotSec16[2];	/*!< ���������������ڴ���65535�������ľ����ֶ�Ϊ0 */
	uint8_t BPB_Media[1];		/*!< ��������� */
	uint8_t BPB_FATSz16[2];		/*!< ÿ��FAT��ռ�������� */
	uint8_t BPB_SecPerTrk[2];	/*!< ÿ�ŵ����������� */
	uint8_t BPB_NumHeads[2];	/*!< ���ͷ�� */
	uint8_t BPB_HiddSec[4];		/*!< �����ش�ͷ�� */
	uint8_t BPB_TotSec32[4];	/*!< ����������������65535�������ľ��ñ��ֶα�ʾ */
}BPBCommon;//=>Fat12/16/32���ò��֡�

typedef struct _BPBFat16
{
	uint8_t BS_DrvNum[1];		/*!< ��������� */
	uint8_t BS_Reserved1[1];	/*!< �����ֶ� */
	uint8_t BS_BootSig[1];		/*!< ������չ��������ǩ��WindowsҪ��ñ�ǩΪ0x28����0x29 */
	uint8_t BS_VolID[4];		/*!< ���̾�ID */
	uint8_t BS_VolLab[11];		/*!< ���̾�� */
	uint8_t BS_FilSysType[8];	/*!< �����ϵ��ļ�ϵͳ���� */
}BPBFat16;//=>Fat16���ⲿ�֡�

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
}BPBFat32;//=>Fat32���ⲿ�֡�

typedef struct _BPB
{
	BPBCommon Common;
	union
	{
		BPBFat16 Fat16;
		BPBFat32 Fat32;
	}Special;
	uint8_t Interval[420];		/*!< ӦΪFat32��ʽ���������� */
	uint8_t EndFlag[2];			/*!< DBR����ǩ�� */
}BPB;//=>512����IMGͷ����

int CheckIMGIdentifier(const File *mapFile);

int CheckIMGFileSystem(const File *mapFile);

FS_TYPE GetIMGType(const BPB *pcBPB);

void IMGTestUnit(const File *mapFile);

#endif