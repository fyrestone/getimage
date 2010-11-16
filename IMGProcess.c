#include <assert.h>
#include "IMGProcess.h"
#include "ColorPrint.h"

int CheckIMGIdentifier(const File *mapFile)
{
	int retVal = IMAGE_PROCESS_FAILED;

	if(CHECK_FILE_PTR_IS_VALID(mapFile))
	{
		const char *beginOfFile = (const char *)mapFile->pvFile;

		if(mapFile->size >= 3)/*identifier����Ϊ3*/
		{
			//EB??90 || E9????
			if((*beginOfFile == (const char)0xEB && *(beginOfFile + 2) == (const char)0x90)
				|| *beginOfFile == (const char)0xE9)
				retVal = IMAGE_PROCESS_SUCCESS;
		}
	}

	return retVal;
}

int CheckIMGFileSystem(const File *mapFile)
{
	if(CHECK_FILE_PTR_IS_VALID(mapFile))
	{
		const char *beginOfFile = (const char *)mapFile->pvFile;

		if(mapFile->size >= 512)
		{
			/*��0x55AA��β����Ϊ��ΪBR*/
			if(LD_UINT16(beginOfFile + 510) == (uint16_t)0xAA55)
			{
				const BPB *pcBPB = (const BPB *)beginOfFile;

				/*��⡰FAT���ַ���*/
				if((LD_UINT32(pcBPB->Special.Fat16.BS_FilSysType) & 0xFFFFFF) == 0x544146)
					return IMAGE_PROCESS_SUCCESS;//���ɹ�

				if((LD_UINT32(pcBPB->Special.Fat32.BS_FilSysType) & 0xFFFFFF) == 0x544146)
					return IMAGE_PROCESS_SUCCESS;//���ɹ�
			}
		}
	}

	return IMAGE_PROCESS_FAILED;//���ʧ��
}

FS_TYPE GetIMGType(const BPB *pcBPB)
{
	FS_TYPE retVal = FAT_ERR;

	/*!
		FAT������������TotalSec����FAT����ռ��������NumFats * FatSz����������ռ��������DataSec����
		��Ŀ¼��ռ��������RootDirSec������FAT32�ĸ�Ŀ¼���ͬ����ͨ�ļ���FAT32��RootDirSectorsΪ0����
		������������RsvdSecCnt���Ĳ��ֹ��ɡ�
	*/
	if(pcBPB)
	{
		uint32_t totalSec;		/*!< �������� */
		uint32_t rsvdSec;		/*!< ����������������FATͷ��512�ֽڣ� */
		uint32_t fatSec;		/*!< FAT�ļ��������ռ������ */
		uint32_t rootDirSec;	/*!< ��Ŀ¼��ռ������ */
		uint32_t dataSec;		/*!< ������ռ����������������һ�ص������� */

		/*����totalSec*/
		if(LD_UINT16(pcBPB->Common.BPB_TotSec16))
			totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
		else
			totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

		/*����rsvdSec*/
		rsvdSec = LD_UINT16(pcBPB->Common.BPB_RsvdSecCnt);

		/*����fatSec*/
		if(LD_UINT16(pcBPB->Common.BPB_FATSz16))
			fatSec = LD_UINT8(pcBPB->Common.BPB_NumFATs) * LD_UINT16(pcBPB->Common.BPB_FATSz16);
		else
			fatSec = LD_UINT8(pcBPB->Common.BPB_NumFATs) * LD_UINT32(pcBPB->Special.Fat32.BPB_FATSz32);

		/*����rootDirSec*/
		rootDirSec = ((LD_UINT16(pcBPB->Common.BPB_RootEntCnt) * 32) + (LD_UINT16(pcBPB->Common.BPB_BytsPerSec) - 1))
			/ LD_UINT16(pcBPB->Common.BPB_BytsPerSec);

		/*����dataSec*/
		dataSec = totalSec - rsvdSec - fatSec - rootDirSec;

		assert(dataSec);

		retVal = FAT12;
		if(dataSec >= MIN_FAT16)
			retVal = FAT16;
		if(dataSec >= MIN_FAT32)
			retVal = FAT32;
	}

	return retVal;
}

void IMGTestUnit(const File *mapFile)
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

