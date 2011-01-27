/*!
\file ISOProcess.c
\author LiuBao
\version 1.0
\date 2010/12/24
\brief ISO������
*/
#include <string.h>		/*< memcmp */
#include <assert.h>
#include "ProjDef.h"
#include "ISOProcess.h"

int JumpToISOPrimVolDesc(media_t media)
{
	static const char *ISO9660Identifier = "CD001";

	int retVal = FAILED;

	/* ��ת�������ISO��ʼ��16������ */
	if(SeekMedia(media, SECTOR_SIZE * 16, MEDIA_SET) == SUCCESS)
	{
		media_access access;
		if(GetMediaAccess(media, &access, sizeof(PrimVolDesc)) == SUCCESS)
		{
			/* ���PrimVolDesc��ʶ���� */
			if(*access.begin == 1)
			{
				const PrimVolDesc *pcPVD = (const PrimVolDesc *)access.begin;

				/* ���ISO9660��� */
				if(!memcmp(pcPVD->ISO9660Identifier, ISO9660Identifier, strlen(ISO9660Identifier)))
					retVal = SUCCESS;
			}
		}
	}

	return retVal;
}

int JumpToISOBootRecordVolDesc(media_t media)
{
	static const char *BootSystemIdentifier = "EL TORITO SPECIFICATION";

	int retVal = FAILED;

	/* ��ת��PrimVolDesc��BootRecordVolDesc����ʼ�� */
	if(SeekMedia(media, sizeof(PrimVolDesc), MEDIA_CUR) == SUCCESS)
	{
		media_access access;
		if(GetMediaAccess(media, &access, sizeof(BootRecordVolDesc)) == SUCCESS)
		{
			/* ���BootRecordVolDesc��ʶ���� */
			if(*access.begin == 0)
			{
				const BootRecordVolDesc *pcBRVD = (const BootRecordVolDesc *)access.begin;

				/* ����Ƿ����EL TORITO�����淶 */
				if(!memcmp(pcBRVD->BootSysID, BootSystemIdentifier, strlen(BootSystemIdentifier)))
					retVal = SUCCESS;
			}
		}
	}

	return retVal;
}

int JumpToISOValidationEntry(media_t media)
{
	int retVal = FAILED;

	media_access access;
	uint32_t absoluteOffset = 0;//ValidationEntry�ľ���ƫ����

	/* ��BootRecordVolDesc�м����ValidationEntry��λ�ã�����ƫ������ */
	if(GetMediaAccess(media, &access, sizeof(BootRecordVolDesc)) == SUCCESS)
	{
		const BootRecordVolDesc *pcBRVD = (const BootRecordVolDesc *)access.begin;

		uint32_t offsetValue = LD_UINT32(pcBRVD->SecToBootCat);
		absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
	}

	/* ��ת��ValidationEntry */
	if(absoluteOffset && SeekMedia(media, absoluteOffset, MEDIA_SET) == SUCCESS)
	{
		if(GetMediaAccess(media, &access, sizeof(ValidationEntry)) == SUCCESS)
		{
			/* ���ValidationEntry��ʶ���� */
			if(*access.begin == 1)
			{
				const ValidationEntry *pcVE = (const ValidationEntry *)access.begin;

				/* ���ValidationEntry��β��� */
				if(LD_UINT16(pcVE->EndofVE) == (uint16_t)0xAA55)
					retVal = SUCCESS;
			}
		}
	}

	return retVal;
}

int JumpToISOInitialEntry(media_t media)
{
	int retVal = FAILED;

	/* ��ת��ValidationEntry��InitialEntry����ʼ�� */
	if(SeekMedia(media, sizeof(ValidationEntry), MEDIA_CUR) == SUCCESS)
	{
		media_access access;
		if(GetMediaAccess(media, &access, sizeof(InitialEntry)) == SUCCESS)
		{
			const InitialEntry *pcIE = (const InitialEntry *)access.begin;

			/*! ���������� */
			if(pcIE->BootIndicator == 0x88)
				retVal = SUCCESS;
		}
	}

	return retVal;
}

int JumpToISOBootableImage(media_t media)
{
	int retVal = FAILED;

	media_access access;
	uint32_t absoluteOffset = 0;//BootImage�ľ���ƫ����

	/* ��InitialEntry�м����BootImage��λ�ã�����ƫ������ */
	if(GetMediaAccess(media, &access, sizeof(InitialEntry)) == SUCCESS)
	{
		const InitialEntry *pcIE = (const InitialEntry *)access.begin;

		uint32_t offsetValue = LD_UINT32(pcIE->LoadRBA);
		absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
	}

	/* ��ת��BootImage */
	if(absoluteOffset && SeekMedia(media, absoluteOffset, MEDIA_SET) == SUCCESS)
		retVal = SUCCESS;

	return retVal;
}

const _TCHAR *GetISOPlatformID(media_t media)
{
	static const _TCHAR *Platform[] = 
	{
		_T("80x86"),
		_T("Power PC"),
		_T("Mac")
	};

	media_access access;

	/* ��ValidationEntry�еõ�ƽ̨ID */
	if(GetMediaAccess(media, &access, sizeof(ValidationEntry)) == SUCCESS)
	{
		const ValidationEntry *pcVE = (const ValidationEntry *)access.begin;

		uint8_t index = pcVE->PlatformID;

		if(index >= 0 && index <= 2)
			return Platform[index];
	}

	return NULL;
}

const _TCHAR *GetISOBootMediaType(media_t media)
{
	static const _TCHAR *BootMediaType[] =
	{
		_T("��ģ��"),
		_T("1.2M ����ģ��"),
		_T("1.44M ����ģ��"),
		_T("2.88M ����ģ��"),
		_T("Ӳ��ģ��")
	};

	media_access access;

	/* ��InitialEntry�еõ������������� */
	if(GetMediaAccess(media, &access, sizeof(InitialEntry)) == SUCCESS)
	{
		const InitialEntry *pcIE = (const InitialEntry *)access.begin;

		uint8_t index = pcIE->BootMediaType;

		if(index >= 0 && index <= 4)
			return BootMediaType[index];
	}

	return NULL;
}
