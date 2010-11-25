#include <Windows.h>	/*!< ʹ��MAX_PATH�� */
#include <stdio.h>		/*!< ʹ��FILE */
#include <assert.h>
#include "ColorPrint.h"
#include "Process.h"
#include "IMGProcess.h"
#include "ISOProcess.h"

/*! ˽��ȫ�ֱ������� */
static char PATH[MAX_PATH];
static const char * const leftMargin = "    ";

/*! ˽�к������� */
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
		
		retVal = PATH;//��ȡ���·��
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
			retVal = PROCESS_SUCCESS;//д��ɹ�
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

		/*������תָ��*/
		if((pcPVD = JumpToISOPrimVolDesc(mapFile)) &&
			(CheckISOIdentifier(pcPVD) == ISO_PROCESS_SUCCESS) &&		/*!< ���ISO��� */
			(pcBRVD = JumpToISOBootRecordVolDesc(mapFile, pcPVD)) &&
			(CheckISOBootIdentifier(pcBRVD) == ISO_PROCESS_SUCCESS) &&	/*!< ��������̱�� */
			(pcVE = JumpToISOValidationEntry(mapFile, pcBRVD)) &&
			(pcIE = JumpToISOInitialEntry(mapFile, pcVE)) &&
			(CheckISOIsBootable(pcIE) == ISO_PROCESS_SUCCESS))			/*!< ����Ƿ�������� */
		{
			const void *entryPtr = JumpToISOBootableImage(mapFile, pcIE);

			entry->pvFile = (void *)(entryPtr);
			entry->size = mapFile->size - ((char *)entryPtr - (char *)mapFile->pvFile);
		}

		/*�����ת���entry�Ƿ���Ч*/
		if(CHECK_FILE_PTR_IS_VALID(entry))
		{
			/*���Դ�ISO��BootImage�����ת��IMG���*/
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

			if(CHECK_FILE_PTR_IS_VALID(entry) &&							/*!< ָ����Ч */
				(CheckIMGIdentifier(entry) == IMAGE_PROCESS_SUCCESS) &&	/*!< �ļ���ʶ��Ч */
				(CheckIMGFileSystem(entry) == IMAGE_PROCESS_SUCCESS) &&	/*!< �ļ�ϵͳ��� */
				(CHECK_PTR_SPACE_IS_VALID(entry, entry->pvFile, 512)))		/*!< BPB�ɶ���� */
				retVal = PROCESS_SUCCESS;//�ҵ�IMG���
		}

	}

	return retVal;
}

MEDIA_TYPE GetInputType(const File *mapFile)
{
	MEDIA_TYPE retVal = UNKNOWN;

	if(CHECK_FILE_PTR_IS_VALID(mapFile))
	{
		//����Ƿ�ΪIMG
		if(CheckIMGIdentifier(mapFile) == IMAGE_PROCESS_SUCCESS)
			retVal = IMG;//��IMG��ʽ
		else
		{
			const PrimVolDesc *pcPVD = JumpToISOPrimVolDesc(mapFile);

			if(pcPVD && CheckISOIdentifier(pcPVD) == ISO_PROCESS_SUCCESS)
				retVal = ISO;//��ISO��ʽ
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
		ColorPrintf(WHITE, "���Դ�ISO�ļ�����ȡIMG:\n\n");
		ColorPrintf(WHITE, "%s����Ѱ��IMG���\t\t\t\t", leftMargin);

		if(GetBootEntryFromISO(mapFile, &entry) == PROCESS_SUCCESS)
		{
			ColorPrintf(LIME, "�ɹ�\n");
			ColorPrintf(WHITE, "%s����д��IMG�ļ�\t\t\t\t", leftMargin);

			if(WriteIMGToFile(&entry, imgPath) == PROCESS_SUCCESS)
			{
				ColorPrintf(LIME, "�ɹ�\n");
				ColorPrintf(WHITE, "\n�������\n");

				retVal = PROCESS_SUCCESS;//д��ɹ���
			}
			else
				ColorPrintf(RED, "ʧ��\n");
		}
		else
			ColorPrintf(RED, "ʧ��\n");
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

		ColorPrintf(WHITE, "��⵽ISO�ļ���Ϣ���£�\n\n");

		/*! ���PrimVolDesc����Ϣ */
		if((pcPVD = JumpToISOPrimVolDesc(mapFile)))
		{
			const ISO9660TimeStr *createDate = (const ISO9660TimeStr *)(pcPVD->VolCreationDate);
			const ISO9660TimeStr *modifyDate = (const ISO9660TimeStr *)(pcPVD->VolModifDate);

			uint32_t volBlocks = LD_UINT32(pcPVD->VolSpaceSz);
			uint16_t bytsPerBlocks = LD_UINT16(pcPVD->LogicalBlockSz);

			ColorPrintf(WHITE, "%s���̱�ǩ��\t\t\t\t", leftMargin);
			ColorPrintf(LIME, "%.32s\n", pcPVD->VolID);
			ColorPrintf(YELLOW, "%s���߼�������\t\t\t", leftMargin);
			ColorPrintf(AQUA, "%u\n", volBlocks);
			ColorPrintf(YELLOW, "%sÿ�߼����ֽ�����\t\t\t", leftMargin);
			ColorPrintf(AQUA, "%u\n", bytsPerBlocks);
			ColorPrintf(YELLOW, "%s����������\t\t\t\t", leftMargin);
			ColorPrintf(AQUA, "%u\n", volBlocks * bytsPerBlocks);

			if(createDate && modifyDate)
			{
				ColorPrintf(WHITE, "%s�������ڣ�\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%.4s/%.2s/%.2s %.2s:%.2s:%.2s\n",
					createDate->Year, createDate->Month, createDate->Day,
					createDate->Hour, createDate->Minute, createDate->Second);

				ColorPrintf(WHITE, "%s�޸����ڣ�\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%.4s/%.2s/%.2s %.2s:%.2s:%.2s\n",
					createDate->Year, createDate->Month, createDate->Day,
					createDate->Hour, createDate->Minute, createDate->Second);
			}
		}

		/*! ���BootRecordVolDesc����Ϣ */
		if((pcBRVD = JumpToISOBootRecordVolDesc(mapFile, pcPVD)) &&
			(CheckISOBootIdentifier(pcBRVD) == ISO_PROCESS_SUCCESS))
		{
			ColorPrintf(YELLOW, "%s�����淶��\t\t\t\t", leftMargin);
			ColorPrintf(AQUA, "EL TORITO\n");

			/*! ���ValidationEntry����Ϣ */
			if((pcVE = JumpToISOValidationEntry(mapFile, pcBRVD)))
			{
				ColorPrintf(WHITE, "%s֧��ƽ̨��\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%s\n", GetISOPlatformID(pcVE));
			}

			/*! ���InitialEntry����Ϣ */
			if((pcIE = JumpToISOInitialEntry(mapFile, pcVE)))
			{
				ColorPrintf(WHITE, "%s�������ͣ�\t\t\t\t", leftMargin);
				if(CheckISOIsBootable(pcIE) == ISO_PROCESS_SUCCESS)
					ColorPrintf(LIME, "����������\n");
				else
					ColorPrintf(LIME, "����������\n");

				ColorPrintf(YELLOW, "%s�����������ͣ�\t\t\t", leftMargin);
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

		ColorPrintf(WHITE, "%sϵͳ�����ƣ�\t\t\t", leftMargin);
		ColorPrintf(LIME, "%.8s\n", pcBPB->Common.BS_OEMName);

		ColorPrintf(WHITE, "%s�����кţ�\t\t\t\t", leftMargin);
		ColorPrintf(LIME, "0x%.4X\n", volID);

		ColorPrintf(WHITE, "%s�������ͣ�\t\t\t\t", leftMargin);
		ColorPrintf(LIME, "0x%.1X\n", LD_UINT8(pcBPB->Common.BPB_Media));

		ColorPrintf(WHITE, "%s�����������ţ�\t\t\t", leftMargin);
		ColorPrintf(LIME, "0x%.2X\n", drvNum);

		ColorPrintf(WHITE, "%s�ļ�ϵͳ��\t\t\t\t", leftMargin);
		ColorPrintf(LIME, "%s\n", fsType);

		ColorPrintf(WHITE, "%sÿ�����ֽ�����\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT16(pcBPB->Common.BPB_BytsPerSec));

		ColorPrintf(WHITE, "%sFAT������\t\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT8(pcBPB->Common.BPB_NumFATs));

		ColorPrintf(WHITE, "%sÿFAT����������\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", secPerFat);

		ColorPrintf(WHITE, "%s������������\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT32(pcBPB->Common.BPB_HiddSec));
		
		ColorPrintf(WHITE, "%sÿ����������\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT8(pcBPB->Common.BPB_SecPerClus));

		ColorPrintf(WHITE, "%s������������\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT16(pcBPB->Common.BPB_RsvdSecCnt));

		ColorPrintf(WHITE, "%s��Ŀ¼������\t\t\t", leftMargin);
		ColorPrintf(LIME, "%u\n", LD_UINT16(pcBPB->Common.BPB_RootEntCnt));

		ColorPrintf(YELLOW, "%s�ŵ�����C����\t\t\t", leftMargin);
		ColorPrintf(AQUA, "%u\n", numTrks);

		ColorPrintf(YELLOW, "%s��ͷ����H����\t\t\t", leftMargin);
		ColorPrintf(AQUA, "%u\n", LD_UINT16(pcBPB->Common.BPB_NumHeads));

		ColorPrintf(YELLOW, "%sÿ�ŵ���������S����\t\t\t", leftMargin);
		ColorPrintf(AQUA, "%u\n", LD_UINT16(pcBPB->Common.BPB_SecPerTrk));

		ColorPrintf(FUCHSIA, "%s����������\t\t\t\t", leftMargin);
		ColorPrintf(AQUA, "%u\n", totalSec);
	}

	return retVal;
}

//========================���Դ��뿪ʼ=============================

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

//========================���Դ������=============================
