/*!
\file Process.c
\author LiuBao
\version 1.0
\date 2011/1/23
\brief ������ʵ��
*/
#include <Windows.h>	/* ʹ��MAX_PATH�� */
#include <stdio.h>		/* ʹ��FILE */
#include <assert.h>
#include "ProjDef.h"
#include "ColorPrint.h"
#include "Process.h"
#include "IMGProcess.h"
#include "ISOProcess.h"

static char PATH[MAX_PATH];						/* GetOutPath�����������ÿ�ε��ö��Ḳ�� */

/*!
������·������iso·�������ȡ����img��д��·�����������룩
/param selfPath iso·��
/param extention ��iso��չ���޸�Ϊ����չ������'.img'
/return ���·��
*/
static const char *GetOutPath(const char *path, const char *extention)
{
	const char *retVal = NULL;

	const char *endOfSelfPath = path + strlen(path);//��ʼ��ָ��·���ִ���β��

	/* �ҵ���չ����ߵġ�.�� */
	while(*endOfSelfPath != '.' && endOfSelfPath != path)
		--endOfSelfPath;

	if(endOfSelfPath != path)
	{
		char *target = PATH;

		/* ����ISO·������չ��ǰ���� */
		while(path != endOfSelfPath)
			*target++ = *path++;
	
		/* д����չ�� */
		while((*target++  = *extention++));
		
		retVal = PATH;//��ȡ���·��
	}

	return retVal;
}

/*!
��img��ʼλ����ȡ��imgд�뵽selfPath·��
\param imageEntry λ����img��ʼ��media_t
\param selfPath imgд��·��
\return �ɹ�����SUCCESS�����򷵻�FAILED
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
			size_t totalSize = 0;//IMG��С

			/* �������IMG��С */
			{
				size_t totalSec = 0;//��������

				if(LD_UINT16(pcBPB->Common.BPB_TotSec16))//�����FAT12/16
					totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
				else//�����FAT32
					totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

				totalSize = totalSec * LD_UINT16(pcBPB->Common.BPB_BytsPerSec);
			}

			/* ��IMGд���ļ� */
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
��ISO�ļ�ͷ����ת������IMGͷ�����ڲ��Զ�����Acronis��������
\param media ISO�ļ���media_t
\return �ɹ�����SUCCESS�����򷵻�FAILED
*/
static int JumpToISOBootEntry(media_t media)
{
	int retVal = FAILED;

	if(media)
	{
		/*������תָ��*/
		if(JumpToISOPrimVolDesc(media) == SUCCESS &&
			JumpToISOBootRecordVolDesc(media) == SUCCESS &&
			JumpToISOValidationEntry(media) == SUCCESS &&
			JumpToISOInitialEntry(media) == SUCCESS &&
			JumpToISOBootableImage(media) == SUCCESS)
		{

			/*���Դ�ISO��BootImage�����ת��IMG���*/
			if(CheckIMGIdentifier(media) != SUCCESS)
			{
				media_access access;
				if(GetMediaAccess(media, &access, 512) == SUCCESS)
				{
					/* Acronis����������ת */
					uint32_t offsetValue = LD_UINT8(access.begin + 244);
					uint32_t relativeOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
					
					(void)SeekMedia(media, relativeOffset, MEDIA_CUR);
				}
			}
		}

		/* �������Ƿ�ΪFAT��ʽ */
		{
			media_access access;
			if(GetMediaAccess(media, &access, 512) == SUCCESS)
			{
				if(CheckIMGIdentifier(media) == SUCCESS &&
					CheckIMGFileSystem(media) == SUCCESS)
					retVal = SUCCESS;//�ҵ�IMG���
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
			retVal = IMG;//��IMG��ʽ
		else if(JumpToISOPrimVolDesc(media) == SUCCESS)
		{
			(void)RewindMedia(media);
			retVal = ISO;//��ISO��ʽ
		}
	}
	
	return retVal;
}

int DumpIMGFromISO(media_t media, const char *path)
{
	static const char * const leftMargin = "    ";//����

	int retVal = FAILED;

	const char *imgPath = GetOutPath(path, ".img");//img�ļ����·��

	if(media && imgPath)
	{
		ColorPrintf(WHITE, "���Դ�ISO�ļ�����ȡIMG:\n\n");
		ColorPrintf(WHITE, "%s����Ѱ��IMG���\t\t\t\t", leftMargin);

		do
		{
			/* ��ת��IMGͷ�� */
			if(JumpToISOBootEntry(media) == SUCCESS)
			{
				ColorPrintf(LIME, "�ɹ�\n");
				ColorPrintf(WHITE, "%s����д��IMG�ļ�\t\t\t\t", leftMargin);
			}
			else
			{
				ColorPrintf(RED, "ʧ��\n");
				break;
			}

			/* ��IMGд���ļ� */
			if(WriteIMGToFile(media, imgPath) == SUCCESS)
			{
				ColorPrintf(LIME, "�ɹ�\n");
				ColorPrintf(WHITE, "\n�������\n");
			}
			else
			{
				ColorPrintf(RED, "ʧ��\n");
				break;
			}

			retVal = SUCCESS;//д��ɹ���
		}while(0);
	}

	return retVal;
}

int DisplayISOInfo(media_t media)
{
	static const char * const leftMargin = "    ";//����

	int retVal = FAILED;

	if(media)
	{
		media_access access;

		ColorPrintf(WHITE, "��⵽ISO�ļ���Ϣ���£�\n\n");

		/* ���PrimVolDesc����Ϣ */
		if(JumpToISOPrimVolDesc(media) == SUCCESS)
		{
			if(GetMediaAccess(media, &access, sizeof(PrimVolDesc)) == SUCCESS)
			{
				const PrimVolDesc *pcPVD = (const PrimVolDesc *)access.begin;
				const ISO9660TimeStr *createDate = (const ISO9660TimeStr *)(pcPVD->VolCreationDate);
				const ISO9660TimeStr *modifyDate = (const ISO9660TimeStr *)(pcPVD->VolModifDate);

				uint32_t volBlocks = LD_UINT32(pcPVD->VolSpaceSz);//ISO���߼�����
				uint16_t bytsPerBlocks = LD_UINT16(pcPVD->LogicalBlockSz);//�߼�����ռ�ֽ���

				ColorPrintf(WHITE, "%s���̱�ǩ��\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%.32s\n", pcPVD->VolID);
				ColorPrintf(YELLOW, "%s���߼�������\t\t\t", leftMargin);
				ColorPrintf(AQUA, "%u\n", volBlocks);
				ColorPrintf(YELLOW, "%sÿ�߼����ֽ�����\t\t\t", leftMargin);
				ColorPrintf(AQUA, "%u\n", bytsPerBlocks);
				ColorPrintf(YELLOW, "%s����������\t\t\t\t", leftMargin);
				ColorPrintf(AQUA, "%u\n", volBlocks * bytsPerBlocks);

				/* ����������ں��޸����� */
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
		}

		/* ���BootRecordVolDesc����Ϣ */
		if(JumpToISOBootRecordVolDesc(media) == SUCCESS)
		{
			ColorPrintf(YELLOW, "%s�����淶��\t\t\t\t", leftMargin);
			ColorPrintf(AQUA, "EL TORITO\n");

			/* ���ValidationEntry����Ϣ */
			if(JumpToISOValidationEntry(media) == SUCCESS)
			{
				ColorPrintf(WHITE, "%s֧��ƽ̨��\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "%s\n", GetISOPlatformID(media));
			}

			/* ���InitialEntry����Ϣ */
			if(JumpToISOInitialEntry(media) == SUCCESS)
			{
				ColorPrintf(WHITE, "%s�������ͣ�\t\t\t\t", leftMargin);
				ColorPrintf(LIME, "����������\n");

				ColorPrintf(YELLOW, "%s�����������ͣ�\t\t\t", leftMargin);
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
	int retVal = FAILED;

	media_access access;
	
	if(GetMediaAccess(media, &access, sizeof(BPB)) == SUCCESS)
	{
		const BPB *pcBPB = (const BPB *)access.begin;
		const char *fsType = NULL;	//�ļ�ϵͳ���ͣ�FAT12/16/32��
		uint32_t totalSec;			//��������
		uint32_t numTrks;			//�ŵ���
		uint32_t volID = 0;			//���
		uint32_t secPerFat = 0;		//ÿ��FAT����ռ������
		uint8_t drvNum = 0;			//��������

		/* ������������ */
		if(LD_UINT16(pcBPB->Common.BPB_TotSec16))
			totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
		else
			totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

		/* ����ŵ��� */
		numTrks = totalSec / 
			(LD_UINT16(pcBPB->Common.BPB_NumHeads) * LD_UINT16(pcBPB->Common.BPB_SecPerTrk));

		/* ��ȡ��ꡢ�������š�ÿFAT���������Լ��ļ�ϵͳ���� */
		switch(GetIMGType(pcBPB))
		{
		case FAT12:
			volID = LD_UINT32(pcBPB->Special.Fat16.BS_VolID);
			drvNum = LD_UINT8(pcBPB->Special.Fat16.BS_DrvNum);
			secPerFat = LD_UINT16(pcBPB->Common.BPB_FATSz16);
			fsType = "FAT12";
			break;
		case FAT16:
			volID = LD_UINT32(pcBPB->Special.Fat16.BS_VolID);
			drvNum = LD_UINT8(pcBPB->Special.Fat16.BS_DrvNum);
			secPerFat = LD_UINT16(pcBPB->Common.BPB_FATSz16);
			fsType = "FAT16";
			break;
		case FAT32:
			volID = LD_UINT32(pcBPB->Special.Fat32.BS_VolID);
			drvNum = LD_UINT8(pcBPB->Special.Fat32.BS_DrvNum);
			secPerFat = LD_UINT32(pcBPB->Special.Fat32.BPB_FATSz32);
			fsType = "FAT32";
			break;
		}

		ColorPrintf(WHITE, "\tϵͳ�����ƣ�");
		ColorPrintf(LIME, "%.8s", pcBPB->Common.BS_OEMName);

		ColorPrintf(WHITE, "\t\t�ļ�ϵͳ��");
		ColorPrintf(LIME, "%s\n", fsType);

		ColorPrintf(WHITE, "\t�����кţ�");
		ColorPrintf(LIME, "0x%.4X", volID);

		ColorPrintf(WHITE, "\t\t�������ͣ�");
		ColorPrintf(LIME, "0x%.1X\n", LD_UINT8(pcBPB->Common.BPB_Media));

		ColorPrintf(WHITE, "\t�����������ţ�");
		ColorPrintf(LIME, "0x%.2X", drvNum);

		ColorPrintf(WHITE, "\t\tÿ�����ֽ�����");
		ColorPrintf(LIME, "%u\n", LD_UINT16(pcBPB->Common.BPB_BytsPerSec));

		ColorPrintf(WHITE, "\t������������");
		ColorPrintf(LIME, "%u", LD_UINT32(pcBPB->Common.BPB_HiddSec));

		ColorPrintf(YELLOW, "\t\t\t�ŵ�����C����");
		ColorPrintf(AQUA, "%u\n", numTrks);

		ColorPrintf(WHITE, "\tÿ����������");
		ColorPrintf(LIME, "%u", LD_UINT8(pcBPB->Common.BPB_SecPerClus));

		ColorPrintf(YELLOW, "\t\t\t��ͷ����H����");
		ColorPrintf(AQUA, "%u\n", LD_UINT16(pcBPB->Common.BPB_NumHeads));

		ColorPrintf(WHITE, "\t������������");
		ColorPrintf(LIME, "%u", LD_UINT16(pcBPB->Common.BPB_RsvdSecCnt));

		ColorPrintf(YELLOW, "\t\t\tÿ�ŵ���������S����");
		ColorPrintf(AQUA, "%u\n", LD_UINT16(pcBPB->Common.BPB_SecPerTrk));

		ColorPrintf(WHITE, "\tFAT������");
		ColorPrintf(LIME, "%u", LD_UINT8(pcBPB->Common.BPB_NumFATs));

		ColorPrintf(WHITE, "\t\t\tÿFAT����������");
		ColorPrintf(LIME, "%u\n", secPerFat);

		ColorPrintf(WHITE, "\t��Ŀ¼������");
		ColorPrintf(LIME, "%u", LD_UINT16(pcBPB->Common.BPB_RootEntCnt));

		ColorPrintf(FUCHSIA, "\t\t\t����������");
		ColorPrintf(AQUA, "%u\n", totalSec);
	}

	return retVal;
}
