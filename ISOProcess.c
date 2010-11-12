#include <string.h>		/*< memcmp */
#include <assert.h>
#include "ISOProcess.h"
#include "ColorPrint.h"

/*! ˽�к������� */
static int TestPrimVolDesc(const PrimVolDesc *pcPVD);
static int TestBootRecordVolDesc(const BootRecordVolDesc *pcBRVD);
static int TestValidationEntry(const ValidationEntry *pcVE);
static int TestInitialEntry(const InitialEntry *pcIE);

const PrimVolDesc *JumpToPrimVolDesc(const File *mapFile)
{
	const PrimVolDesc *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile))
		retVal = (const PrimVolDesc *)JUMP_PTR_BY_LENGTH(mapFile, mapFile->pvFile, SECTOR_SIZE * 16);

	return CHECK_PTR_SPACE_IS_VALID(mapFile, retVal, sizeof(PrimVolDesc)) ? retVal : NULL;
	///*! ���ָ���Ƿ�Խ��*/
	//if(mapFile->size > SECTOR_SIZE * 16 + sizeof(PrimVolDesc))
	//	return (const PrimVolDesc *)((const char *)mapFile->pvFile + SECTOR_SIZE * 16);
	//else
	//	return NULL;
}

const BootRecordVolDesc *JumpToBootRecordVolDesc(const File *mapFile, const PrimVolDesc *pcPVD)
{
	//assert(CHECK_FILE_PTR_IS_VALID(mapFile) && pcPVD);
	const BootRecordVolDesc *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && pcPVD)
		retVal = (const BootRecordVolDesc *)JUMP_PTR_BY_LENGTH(mapFile, pcPVD, sizeof(PrimVolDesc));

	return CHECK_PTR_SPACE_IS_VALID(mapFile, retVal, sizeof(BootRecordVolDesc)) ? retVal : NULL;

	///*! ����ת����17����*/
	//if(mapFile->size > SECTOR_SIZE * 16 + sizeof(PrimVolDesc) + sizeof(BootRecordVolDesc))
	//	return (const BootRecordVolDesc *)((const char *)pcPVD + sizeof(PrimVolDesc));
	//else
	//	return NULL;
}

const ValidationEntry *JumpToValidationEntry(const File *mapFile, const BootRecordVolDesc *pcBRVD)
{
	const ValidationEntry *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && pcBRVD)
	{
		uint32_t offsetValue = LD_UINT32(pcBRVD->SecToBootCat);
		uint32_t absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;

		retVal = (const ValidationEntry *)JUMP_PTR_BY_LENGTH(mapFile, mapFile->pvFile, absoluteOffset);
	}

	return CHECK_PTR_SPACE_IS_VALID(mapFile, retVal, sizeof(ValidationEntry)) ? retVal : NULL;
	//assert(CHECK_FILE_PTR_IS_VALID(mapFile) && pcBRVD);

	//{
	//	uint32_t absoluteOffset = LD_UINT32(pcBRVD->SecToBootCat);

	//	if(mapFile->size > SECTOR_SIZE * absoluteOffset + sizeof(ValidationEntry))
	//		return (const ValidationEntry *)((const char *)mapFile->pvFile + SECTOR_SIZE * absoluteOffset);
	//	else
	//		return NULL;
	//}
}

const InitialEntry *JumpToInitialEntry(const File *mapFile, const ValidationEntry *pcVE)
{
	const InitialEntry *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && pcVE)
		retVal = (const InitialEntry *)JUMP_PTR_BY_LENGTH(mapFile, pcVE, sizeof(ValidationEntry));

	return CHECK_PTR_SPACE_IS_VALID(mapFile, retVal, sizeof(InitialEntry)) ? retVal : NULL;
	//assert(CHECK_FILE_PTR_IS_VALID(mapFile) && pcVE);

	//if(mapFile->size > ((const char *)pcVE - (const char *)mapFile->pvFile) + sizeof(ValidationEntry))
	//	return (const InitialEntry *)((const char *)pcVE + sizeof(ValidationEntry));
	//else
	//	return NULL;
}

const void *JumpToBootableImage(const File *mapFile, const InitialEntry *pcIE)
{
	const void *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && pcIE)
	{
		uint32_t offsetValue = LD_UINT32(pcIE->LoadRBA);
		uint32_t absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;

		retVal = JUMP_PTR_BY_LENGTH(mapFile, mapFile->pvFile, absoluteOffset);
	}

	return retVal;
	//assert(CHECK_FILE_PTR_IS_VALID(mapFile) && pcIE);

	//{
	//	uint32_t absoluteOffset = LD_UINT32(pcIE->LoadRBA);

	//	if(mapFile->size > SECTOR_SIZE * absoluteOffset)
	//		return (const void *)((const char *)mapFile->pvFile + SECTOR_SIZE * absoluteOffset);
	//	else
	//		return NULL;
	//}
}

static int TestPrimVolDesc(const PrimVolDesc *pcPVD)
{
	int retVal = ISO_PROCESS_FAILED;

	do
	{
		if(sizeof(PrimVolDesc) != 2048)
			break;

		/*! �����PrimaryVolumnDescriptor��PrimaryIndicatorӦΪ1 */
		if(pcPVD->PrimaryIndicator != 1)
			break;

		/*! ISO9660IdentifierӦΪCD001 */
		if(CheckISOIdentifier(pcPVD) != ISO_PROCESS_SUCCESS)
			break;

		/*! ��ǰ�����汾��Ϊ1 */
		if(pcPVD->DescVer != 1)
			break;

		retVal = ISO_PROCESS_SUCCESS;
	}while(0);

	return retVal;
}

static int TestBootRecordVolDesc(const BootRecordVolDesc *pcBRVD)
{
	int retVal = ISO_PROCESS_FAILED;

	do
	{
		if(sizeof(BootRecordVolDesc) != 2048)
			break;

		/*! �����PrimaryVolumnDescriptor��PrimaryIndicatorӦΪ0 */
		if(pcBRVD->BootRecordIndic != 0)
			break;

		/*! ����El Torito��׼�������̴˴�ӦΪEL TORITO SPECIFICATION */
		if(CheckBootIdentifier(pcBRVD) != ISO_PROCESS_SUCCESS)
			break;

		/*! ��ǰ�����汾��Ϊ1 */
		if(pcBRVD->DescVer != 1)
			break;

		retVal = ISO_PROCESS_SUCCESS;
	}while(0);

	return retVal;
}

static int TestValidationEntry(const ValidationEntry *pcVE)
{
	int retVal = ISO_PROCESS_FAILED;

	do
	{
		if(sizeof(ValidationEntry) != 32)
			break;

		/*! HeaderID��Ϊ01 */
		if(pcVE->HeaderID != 0x01)
			break;

		/*! �˴�0=80x86��1=Power PC��2=Mac */
		if(pcVE->PlatformID > 2 || pcVE->PlatformID < 0)
			break;

		/*! ��βΪ0x55AA */
		if(LD_UINT16(pcVE->EndofVE) != (uint16_t)0xAA55)
			break;

		retVal = ISO_PROCESS_SUCCESS;
	}while(0);

	return retVal;
}

static int TestInitialEntry(const InitialEntry *pcIE)
{
	int retVal = ISO_PROCESS_FAILED;

	do
	{
		if(sizeof(InitialEntry) != 32)
			break;

		/*! �˴�88=Bootable��00=Not Bootable */
		if(pcIE->BootIndicator != 0x88 && pcIE->BootIndicator != 0x00)
			break;

		/*! 0=No Emulation��1=1.2M��2=1.44M��3=2.88M��4=HardDisk(drive 80) */
		if(pcIE->BootMediaType > 4 || pcIE->BootMediaType < 0)
			break;

		retVal = ISO_PROCESS_SUCCESS;
	}while(0);

	return retVal;
}

int CheckISOIdentifier(const PrimVolDesc *pcPVD)
{
	static const char *ISO9660Identifier = "CD001";

	if(pcPVD && memcmp(pcPVD->ISO9660Identifier, ISO9660Identifier, strlen(ISO9660Identifier)))
		return ISO_PROCESS_FAILED;
	else
		return ISO_PROCESS_SUCCESS;
}

int CheckBootIdentifier(const BootRecordVolDesc *pcBRVD)
{
	static const char *BootSystemIdentifier = "EL TORITO SPECIFICATION";

	if(pcBRVD && memcmp(pcBRVD->BootSysID, BootSystemIdentifier, strlen(BootSystemIdentifier)))
		return ISO_PROCESS_FAILED;
	else
		return ISO_PROCESS_SUCCESS;
}

const char *CheckBootIndicator(const InitialEntry *pcIE)
{
	static const char *BootType[] = 
	{
		"����������",
		"������������"
	};

	const char *retVal = NULL;

	if(pcIE)
	{
		switch(pcIE->BootIndicator)
		{
		case 0x88:
			retVal = BootType[0];//���������̡�
		case 0x00:
			retVal = BootType[1];//�����������̡�
		}
	}

	return retVal;
}

const char *CheckBootMediaType(const InitialEntry *pcIE)
{
	static const char *BootMediaType[] =
	{
		"��ģ��",
		"1.2M ����ģ��",
		"1.44M ����ģ��",
		"2.88M ����ģ��",
		"Ӳ��ģ��"
	};

	if(pcIE)
	{
		uint8_t index = pcIE->BootMediaType;

		if(index >= 0 && index <= 4)
			return BootMediaType[index];
	}

	return NULL;
}

void ISOTestUnit(const File *mapFile)
{
#define TEST_IF_TRUE_SET_ERR_AND_BREAK(condition, errStr)	\
	if(condition)											\
	{														\
		errStrPtr = #errStr;								\
		break;												\
	}																

	const char *errStrPtr = NULL;

	assert(CHECK_FILE_PTR_IS_VALID(mapFile));

	ColorPrintf(AQUA, "����ISOProcessģ�鵥Ԫ���ԣ�\n");

	do
	{
		const PrimVolDesc *pcPVD = NULL;
		const BootRecordVolDesc *pcBRVD = NULL;
		const ValidationEntry *pcVE = NULL;
		const InitialEntry *pcIE = NULL;

		/*! ���Դ������ */
		ColorPrintf(WHITE, "���ڲ��Դ������\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!CHECK_FILE_PTR_IS_VALID(mapFile),
			��Ԫ���Դ������Ϊ��)
		ColorPrintf(GREEN, "ͨ��\n");

		/*! ����PrimVolDesc */
		ColorPrintf(WHITE, "���ڲ���PrimVolDesc\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcPVD = JumpToPrimVolDesc(mapFile)),//��ת��PrimVolDesc
			JumpToPrimVolDesc��ת��NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestPrimVolDesc(pcPVD) != ISO_PROCESS_SUCCESS,
			TestPrimVolDesc���ʧ��)
		ColorPrintf(GREEN, "ͨ��\n");

		/*! ����BootRecordVolDesc */
		ColorPrintf(WHITE, "���ڲ���BootRecordVolDesc\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcBRVD = JumpToBootRecordVolDesc(mapFile, pcPVD)),//��ת��BootRecordVolDesc
			JumpToBootRecordVolDesc��ת��NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestBootRecordVolDesc(pcBRVD) != ISO_PROCESS_SUCCESS,
			TestBootRecordVolDesc���ʧ��)
		ColorPrintf(GREEN, "ͨ��\n");

		/*! ����ValidationEntry */
		ColorPrintf(WHITE, "���ڲ���ValidationEntry\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcVE = JumpToValidationEntry(mapFile, pcBRVD)),//��ת��ValidationEntry
			JumpToValidationEntry��ת��NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestValidationEntry(pcVE) != ISO_PROCESS_SUCCESS,
			TestValidationEntry���ʧ��)
		ColorPrintf(GREEN, "ͨ��\n");

		/*! ����InitialEntry */
		ColorPrintf(WHITE, "���ڲ���InitialEntry\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcIE = JumpToInitialEntry(mapFile, pcVE)),//��ת��InitialEntry
			JumpToInitialEntry��ת��NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestInitialEntry(pcIE) != ISO_PROCESS_SUCCESS,
			TestInitialEntry���ʧ��)
		ColorPrintf(GREEN, "ͨ��\n");

		/*! ������� */
		ColorPrintf(GREEN, "ȫ������ͨ����\n");
	}while(0);

	if(errStrPtr)//�����д�����
	{
		ColorPrintf(RED, "%s\n", errStrPtr);
	}

#undef TEST_IF_TRUE_SET_ERR_AND_BREAK
}
