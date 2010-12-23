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
		/*��0x55AA��β����Ϊ��ΪBR*/
		if(LD_UINT16(access.begin + 510) == (uint16_t)0xAA55)
		{
			const BPB *pcBPB = (const BPB *)access.begin;

			/*��⡰FAT���ַ���*/
			if((LD_UINT32(pcBPB->Special.Fat16.BS_FilSysType) & 0xFFFFFF) == 0x544146)
				return IMAGE_PROCESS_SUCCESS;//���ɹ�

			if((LD_UINT32(pcBPB->Special.Fat32.BS_FilSysType) & 0xFFFFFF) == 0x544146)
				return IMAGE_PROCESS_SUCCESS;//���ɹ�
		}
	}

	return IMAGE_PROCESS_FAILED;//���ʧ��
}

FS_TYPE GetIMGType(const BPB *pcBPB)
{
	FS_TYPE retVal = FAT_ERR;

	/*!
		FAT������������TotalSec���ɾ�������������RsvdSecCnt����FAT����ռ��������NumFats * FatSz����
		��Ŀ¼����ռ��������RootDirSec������FAT32�ĸ�Ŀ¼���ͬ����ͨ�ļ���FAT32��RootDirSectorsΪ0����
		������ռ��������DataSec���Ĳ������ι��ɡ����б���������������FATͷ����BPB��
	*/
	if(pcBPB)
	{
		uint32_t totalSec;		/*!< �������� */
		uint32_t rsvdSec;		/*!< ����������������FATͷ��512�ֽڣ� */
		uint32_t fatSec;		/*!< FAT�ļ��������ռ������ */
		uint32_t rootDirSec;	/*!< ��Ŀ¼��ռ������ */
		uint32_t dataSec;		/*!< ������ռ����������������һ�ص������� */
		uint32_t dataClusters;	/*!< ������ռ���� */

		/* ����totalSec */
		if(LD_UINT16(pcBPB->Common.BPB_TotSec16))
			totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
		else
			totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

		/* ����rsvdSec */
		rsvdSec = LD_UINT16(pcBPB->Common.BPB_RsvdSecCnt);

		/* ����fatSec */
		if(LD_UINT16(pcBPB->Common.BPB_FATSz16))
			fatSec = LD_UINT8(pcBPB->Common.BPB_NumFATs) * LD_UINT16(pcBPB->Common.BPB_FATSz16);
		else
			fatSec = LD_UINT8(pcBPB->Common.BPB_NumFATs) * LD_UINT32(pcBPB->Special.Fat32.BPB_FATSz32);

		/* ����rootDirSec */
		rootDirSec = ((LD_UINT16(pcBPB->Common.BPB_RootEntCnt) * 32) + (LD_UINT16(pcBPB->Common.BPB_BytsPerSec) - 1))
			/ LD_UINT16(pcBPB->Common.BPB_BytsPerSec);

		/* ����dataSec */
		dataSec = totalSec - rsvdSec - fatSec - rootDirSec;

		/* ����dataClusters */
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

//========================���Դ��뿪ʼ=============================

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

	ColorPrintf(AQUA, "����IMG���ԣ�\n");

	do
	{
		ColorPrintf(RED, "�޲���������\n");
	}while(0);

	if(errStrPtr)//�����д�����
	{
		ColorPrintf(RED, "%s\n", errStrPtr);
	}

#undef TEST_IF_TRUE_SET_ERR_AND_BREAK
}

#endif

//========================���Դ������=============================
