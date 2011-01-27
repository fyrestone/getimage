/*!
\file ISOProcess.c
\author LiuBao
\version 1.0
\date 2010/12/24
\brief ISO处理函数
*/
#include <string.h>		/*< memcmp */
#include <assert.h>
#include "ProjDef.h"
#include "ISOProcess.h"

int JumpToISOPrimVolDesc(media_t media)
{
	static const char *ISO9660Identifier = "CD001";

	int retVal = FAILED;

	/* 跳转到相对于ISO起始的16扇区处 */
	if(SeekMedia(media, SECTOR_SIZE * 16, MEDIA_SET) == SUCCESS)
	{
		media_access access;
		if(GetMediaAccess(media, &access, sizeof(PrimVolDesc)) == SUCCESS)
		{
			/* 检查PrimVolDesc的识别标记 */
			if(*access.begin == 1)
			{
				const PrimVolDesc *pcPVD = (const PrimVolDesc *)access.begin;

				/* 检查ISO9660标记 */
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

	/* 跳转到PrimVolDesc后（BootRecordVolDesc的起始） */
	if(SeekMedia(media, sizeof(PrimVolDesc), MEDIA_CUR) == SUCCESS)
	{
		media_access access;
		if(GetMediaAccess(media, &access, sizeof(BootRecordVolDesc)) == SUCCESS)
		{
			/* 检查BootRecordVolDesc的识别标记 */
			if(*access.begin == 0)
			{
				const BootRecordVolDesc *pcBRVD = (const BootRecordVolDesc *)access.begin;

				/* 检查是否符合EL TORITO启动规范 */
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
	uint32_t absoluteOffset = 0;//ValidationEntry的绝对偏移量

	/* 从BootRecordVolDesc中计算出ValidationEntry的位置（绝对偏移量） */
	if(GetMediaAccess(media, &access, sizeof(BootRecordVolDesc)) == SUCCESS)
	{
		const BootRecordVolDesc *pcBRVD = (const BootRecordVolDesc *)access.begin;

		uint32_t offsetValue = LD_UINT32(pcBRVD->SecToBootCat);
		absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
	}

	/* 跳转到ValidationEntry */
	if(absoluteOffset && SeekMedia(media, absoluteOffset, MEDIA_SET) == SUCCESS)
	{
		if(GetMediaAccess(media, &access, sizeof(ValidationEntry)) == SUCCESS)
		{
			/* 检查ValidationEntry的识别标记 */
			if(*access.begin == 1)
			{
				const ValidationEntry *pcVE = (const ValidationEntry *)access.begin;

				/* 检查ValidationEntry结尾标记 */
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

	/* 跳转到ValidationEntry后（InitialEntry的起始） */
	if(SeekMedia(media, sizeof(ValidationEntry), MEDIA_CUR) == SUCCESS)
	{
		media_access access;
		if(GetMediaAccess(media, &access, sizeof(InitialEntry)) == SUCCESS)
		{
			const InitialEntry *pcIE = (const InitialEntry *)access.begin;

			/*! 检查启动标记 */
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
	uint32_t absoluteOffset = 0;//BootImage的绝对偏移量

	/* 从InitialEntry中计算出BootImage的位置（绝对偏移量） */
	if(GetMediaAccess(media, &access, sizeof(InitialEntry)) == SUCCESS)
	{
		const InitialEntry *pcIE = (const InitialEntry *)access.begin;

		uint32_t offsetValue = LD_UINT32(pcIE->LoadRBA);
		absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
	}

	/* 跳转到BootImage */
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

	/* 从ValidationEntry中得到平台ID */
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
		_T("非模拟"),
		_T("1.2M 软盘模拟"),
		_T("1.44M 软盘模拟"),
		_T("2.88M 软盘模拟"),
		_T("硬盘模拟")
	};

	media_access access;

	/* 从InitialEntry中得到启动介质类型 */
	if(GetMediaAccess(media, &access, sizeof(InitialEntry)) == SUCCESS)
	{
		const InitialEntry *pcIE = (const InitialEntry *)access.begin;

		uint8_t index = pcIE->BootMediaType;

		if(index >= 0 && index <= 4)
			return BootMediaType[index];
	}

	return NULL;
}
