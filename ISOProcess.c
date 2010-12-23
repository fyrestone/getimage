#include <string.h>		/*< memcmp */
#include <assert.h>
#include "ISOProcess.h"

int JumpToISOPrimVolDesc(media_t media)
{
	static const char *ISO9660Identifier = "CD001";

	int retVal = ISO_PROCESS_FAILED;

	if(SeekMedia(media, SECTOR_SIZE * 16, MEDIA_SET) == MAP_SUCCESS)
	{
		media_access access;
		if(GetMediaAccess(media, &access, sizeof(PrimVolDesc)) == MAP_SUCCESS)
		{
			/* ���PrimVolDesc��ʶ���� */
			if(*access.begin == 1)
			{
				const PrimVolDesc *pcPVD = (const PrimVolDesc *)access.begin;

				/* ���ISO9660��� */
				if(!memcmp(pcPVD->ISO9660Identifier, ISO9660Identifier, strlen(ISO9660Identifier)))
					retVal = ISO_PROCESS_SUCCESS;
			}
		}
	}

	return retVal;
}

int JumpToISOBootRecordVolDesc(media_t media)
{
	static const char *BootSystemIdentifier = "EL TORITO SPECIFICATION";

	int retVal = ISO_PROCESS_FAILED;

	if(SeekMedia(media, sizeof(PrimVolDesc), MEDIA_CUR) == MAP_SUCCESS)
	{
		media_access access;
		if(GetMediaAccess(media, &access, sizeof(BootRecordVolDesc)) == MAP_SUCCESS)
		{
			/* ���BootRecordVolDesc��ʶ���� */
			if(*access.begin == 0)
			{
				const BootRecordVolDesc *pcBRVD = (const BootRecordVolDesc *)access.begin;

				/* ����Ƿ����EL TORITO�����淶 */
				if(!memcmp(pcBRVD->BootSysID, BootSystemIdentifier, strlen(BootSystemIdentifier)))
					retVal = ISO_PROCESS_SUCCESS;
			}
		}
	}

	return retVal;
}

int JumpToISOValidationEntry(media_t media)
{
	int retVal = ISO_PROCESS_FAILED;

	media_access access;
	uint32_t absoluteOffset;

	if(GetMediaAccess(media, &access, sizeof(BootRecordVolDesc)) == MAP_SUCCESS)
	{
		const BootRecordVolDesc *pcBRVD = (const BootRecordVolDesc *)access.begin;

		uint32_t offsetValue = LD_UINT32(pcBRVD->SecToBootCat);
		absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
	}

	if(SeekMedia(media, absoluteOffset, MEDIA_SET) == MAP_SUCCESS)
	{
		if(GetMediaAccess(media, &access, sizeof(ValidationEntry)) == MAP_SUCCESS)
		{
			/* ���ValidationEntry��ʶ���� */
			if(*access.begin == 1)
			{
				const ValidationEntry *pcVE = (const ValidationEntry *)access.begin;

				/* ���ValidationEntry��β��� */
				if(LD_UINT16(pcVE->EndofVE) == (uint16_t)0xAA55)
					retVal = ISO_PROCESS_SUCCESS;
			}
		}
	}

	return retVal;
}

int JumpToISOInitialEntry(media_t media)
{
	int retVal = ISO_PROCESS_FAILED;

	if(SeekMedia(media, sizeof(ValidationEntry), MEDIA_CUR) == MAP_SUCCESS)
	{
		media_access access;
		if(GetMediaAccess(media, &access, sizeof(InitialEntry)) == MAP_SUCCESS)
		{
			const InitialEntry *pcIE = (const InitialEntry *)access.begin;

			/*! ���������� */
			if(pcIE->BootIndicator == 0x88)
				retVal = ISO_PROCESS_SUCCESS;
		}
	}

	return retVal;
}

int JumpToISOBootableImage(media_t media)
{
	int retVal = ISO_PROCESS_FAILED;

	media_access access;
	uint32_t absoluteOffset;

	if(GetMediaAccess(media, &access, sizeof(BootRecordVolDesc)) == MAP_SUCCESS)
	{
		const InitialEntry *pcIE = (const InitialEntry *)access.begin;

		uint32_t offsetValue = LD_UINT32(pcIE->LoadRBA);
		absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
	}

	if(SeekMedia(media, absoluteOffset, MEDIA_SET) == MAP_SUCCESS)
		retVal = ISO_PROCESS_SUCCESS;

	return retVal;
}

const char *GetISOPlatformID(media_t media)
{
	static const char *Platform[] = 
	{
		"80x86",
		"Power PC",
		"Mac"
	};

	media_access access;

	if(GetMediaAccess(media, &access, sizeof(ValidationEntry)) == MAP_SUCCESS)
	{
		const ValidationEntry *pcVE = (const ValidationEntry *)access.begin;

		uint8_t index = pcVE->PlatformID;

		if(index >= 0 && index <= 2)
			return Platform[index];
	}

	return NULL;
}

const char *GetISOBootMediaType(media_t media)
{
	static const char *BootMediaType[] =
	{
		"��ģ��",
		"1.2M ����ģ��",
		"1.44M ����ģ��",
		"2.88M ����ģ��",
		"Ӳ��ģ��"
	};

	media_access access;
	if(GetMediaAccess(media, &access, sizeof(InitialEntry)) == MAP_SUCCESS)
	{
		const InitialEntry *pcIE = (const InitialEntry *)access.begin;

		uint8_t index = pcIE->BootMediaType;

		if(index >= 0 && index <= 4)
			return BootMediaType[index];
	}

	return NULL;
}
