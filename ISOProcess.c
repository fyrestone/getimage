#include <string.h>		/*< memcmp */
#include <assert.h>
#include "ISOProcess.h"
#include "ColorPrint.h"

const PrimVolDesc *JumpToISOPrimVolDesc(const File *mapFile)
{
	const PrimVolDesc *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile))
		retVal = (const PrimVolDesc *)JUMP_PTR_BY_LENGTH(mapFile, mapFile->pvFile, SECTOR_SIZE * 16);

	/*! �����ÿռ��Լ�PrimVolDesc��ʶ���� */
	if(CHECK_PTR_SPACE_IS_VALID(mapFile, retVal, sizeof(PrimVolDesc)) && retVal->PrimaryIndicator == 0x1)
		return retVal;
	else
		return NULL;
}

const BootRecordVolDesc *JumpToISOBootRecordVolDesc(const File *mapFile, const PrimVolDesc *pcPVD)
{
	const BootRecordVolDesc *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && pcPVD)
		retVal = (const BootRecordVolDesc *)JUMP_PTR_BY_LENGTH(mapFile, pcPVD, sizeof(PrimVolDesc));

	/*! �����ÿռ��Լ�BootRecordVolDesc��ʶ���� */
	if(CHECK_PTR_SPACE_IS_VALID(mapFile, retVal, sizeof(BootRecordVolDesc)) && retVal->BootRecordIndic ==0x0)
		return retVal;
	else
		return 0;
}

const ValidationEntry *JumpToISOValidationEntry(const File *mapFile, const BootRecordVolDesc *pcBRVD)
{
	const ValidationEntry *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && pcBRVD)
	{
		uint32_t offsetValue = LD_UINT32(pcBRVD->SecToBootCat);
		uint32_t absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;

		retVal = (const ValidationEntry *)JUMP_PTR_BY_LENGTH(mapFile, mapFile->pvFile, absoluteOffset);
	}

	/*! �����ÿռ��Լ�ValidationEntry��ʶ���� */
	if(CHECK_PTR_SPACE_IS_VALID(mapFile, retVal, sizeof(ValidationEntry)) && retVal->HeaderID == 0x01)
		return retVal;
	else
		return NULL;
}

const InitialEntry *JumpToISOInitialEntry(const File *mapFile, const ValidationEntry *pcVE)
{
	const InitialEntry *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && pcVE)
		retVal = (const InitialEntry *)JUMP_PTR_BY_LENGTH(mapFile, pcVE, sizeof(ValidationEntry));

	/*! �������ÿռ䣬InitialEntry��ʶ���� */
	return CHECK_PTR_SPACE_IS_VALID(mapFile, retVal, sizeof(InitialEntry)) ? retVal : NULL;
}

const void *JumpToISOBootableImage(const File *mapFile, const InitialEntry *pcIE)
{
	const void *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile) && pcIE)
	{
		uint32_t offsetValue = LD_UINT32(pcIE->LoadRBA);
		uint32_t absoluteOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;

		retVal = JUMP_PTR_BY_LENGTH(mapFile, mapFile->pvFile, absoluteOffset);
	}

	/*! �޷�ȷ��BootImage�������м�� */
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

int CheckISOBootIdentifier(const BootRecordVolDesc *pcBRVD)
{
	static const char *BootSystemIdentifier = "EL TORITO SPECIFICATION";

	if(pcBRVD && memcmp(pcBRVD->BootSysID, BootSystemIdentifier, strlen(BootSystemIdentifier)))
		return ISO_PROCESS_FAILED;
	else
		return ISO_PROCESS_SUCCESS;
}

int CheckISOIsBootable(const InitialEntry *pcIE)
{
	if(pcIE && pcIE->BootIndicator == 0x88)
		return ISO_PROCESS_SUCCESS;
	else
		return ISO_PROCESS_FAILED;
}

const char *GetISOPlatformID(const ValidationEntry *pcVE)
{
	static const char *Platform[] = 
	{
		"80x86",
		"Power PC",
		"Mac"
	};

	if(pcVE)
	{
		uint8_t index = pcVE->PlatformID;

		if(index >= 0 && index <= 2)
			return Platform[index];
	}

	return NULL;
}

const char *GetISOBootMediaType(const InitialEntry *pcIE)
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

//========================���Դ��뿪ʼ=============================

#ifdef _DEBUG

/*! ˽�к������� */
static int TestPrimVolDesc(const PrimVolDesc *pcPVD);
static int TestBootRecordVolDesc(const BootRecordVolDesc *pcBRVD);
static int TestValidationEntry(const ValidationEntry *pcVE);
static int TestInitialEntry(const InitialEntry *pcIE);

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
		if(CheckISOBootIdentifier(pcBRVD) != ISO_PROCESS_SUCCESS)
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
			!(pcPVD = JumpToISOPrimVolDesc(mapFile)),//��ת��PrimVolDesc
			JumpToISOPrimVolDesc��ת��NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestPrimVolDesc(pcPVD) != ISO_PROCESS_SUCCESS,
			TestPrimVolDesc���ʧ��)
		ColorPrintf(GREEN, "ͨ��\n");

		/*! ����BootRecordVolDesc */
		ColorPrintf(WHITE, "���ڲ���BootRecordVolDesc\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcBRVD = JumpToISOBootRecordVolDesc(mapFile, pcPVD)),//��ת��BootRecordVolDesc
			JumpToISOBootRecordVolDesc��ת��NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestBootRecordVolDesc(pcBRVD) != ISO_PROCESS_SUCCESS,
			TestBootRecordVolDesc���ʧ��)
		ColorPrintf(GREEN, "ͨ��\n");

		/*! ����ValidationEntry */
		ColorPrintf(WHITE, "���ڲ���ValidationEntry\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcVE = JumpToISOValidationEntry(mapFile, pcBRVD)),//��ת��ValidationEntry
			JumpToISOValidationEntry��ת��NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestValidationEntry(pcVE) != ISO_PROCESS_SUCCESS,
			TestValidationEntry���ʧ��)
		ColorPrintf(GREEN, "ͨ��\n");

		/*! ����InitialEntry */
		ColorPrintf(WHITE, "���ڲ���InitialEntry\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcIE = JumpToISOInitialEntry(mapFile, pcVE)),//��ת��InitialEntry
			JumpToISOInitialEntry��ת��NULL)
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

#endif

//========================���Դ������=============================
