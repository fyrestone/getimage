#include <string.h>		/*< memcmp */
#include <assert.h>
#include "ISOProcess.h"
#include "ColorPrint.h"

const PrimVolDesc *JumpToISOPrimVolDesc(const File *mapFile)
{
	const PrimVolDesc *retVal = NULL;

	if(CHECK_FILE_PTR_IS_VALID(mapFile))
		retVal = (const PrimVolDesc *)JUMP_PTR_BY_LENGTH(mapFile, mapFile->pvFile, SECTOR_SIZE * 16);

	/*! 检查可用空间以及PrimVolDesc的识别标记 */
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

	/*! 检查可用空间以及BootRecordVolDesc的识别标记 */
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

	/*! 检查可用空间以及ValidationEntry的识别标记 */
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

	/*! 仅检查可用空间，InitialEntry无识别标记 */
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

	/*! 无法确定BootImage，不进行检查 */
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
		"非模拟",
		"1.2M 软盘模拟",
		"1.44M 软盘模拟",
		"2.88M 软盘模拟",
		"硬盘模拟"
	};

	if(pcIE)
	{
		uint8_t index = pcIE->BootMediaType;

		if(index >= 0 && index <= 4)
			return BootMediaType[index];
	}

	return NULL;
}

//========================测试代码开始=============================

#ifdef _DEBUG

/*! 私有函数声明 */
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

		/*! 如果是PrimaryVolumnDescriptor，PrimaryIndicator应为1 */
		if(pcPVD->PrimaryIndicator != 1)
			break;

		/*! ISO9660Identifier应为CD001 */
		if(CheckISOIdentifier(pcPVD) != ISO_PROCESS_SUCCESS)
			break;

		/*! 当前描述版本号为1 */
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

		/*! 如果是PrimaryVolumnDescriptor，PrimaryIndicator应为0 */
		if(pcBRVD->BootRecordIndic != 0)
			break;

		/*! 符合El Torito标准的启动盘此处应为EL TORITO SPECIFICATION */
		if(CheckISOBootIdentifier(pcBRVD) != ISO_PROCESS_SUCCESS)
			break;

		/*! 当前描述版本号为1 */
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

		/*! HeaderID必为01 */
		if(pcVE->HeaderID != 0x01)
			break;

		/*! 此处0=80x86；1=Power PC；2=Mac */
		if(pcVE->PlatformID > 2 || pcVE->PlatformID < 0)
			break;

		/*! 结尾为0x55AA */
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

		/*! 此处88=Bootable；00=Not Bootable */
		if(pcIE->BootIndicator != 0x88 && pcIE->BootIndicator != 0x00)
			break;

		/*! 0=No Emulation；1=1.2M；2=1.44M；3=2.88M；4=HardDisk(drive 80) */
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

	ColorPrintf(AQUA, "运行ISOProcess模块单元测试：\n");

	do
	{
		const PrimVolDesc *pcPVD = NULL;
		const BootRecordVolDesc *pcBRVD = NULL;
		const ValidationEntry *pcVE = NULL;
		const InitialEntry *pcIE = NULL;

		/*! 测试传入参数 */
		ColorPrintf(WHITE, "正在测试传入参数\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!CHECK_FILE_PTR_IS_VALID(mapFile),
			单元测试传入参数为空)
		ColorPrintf(GREEN, "通过\n");

		/*! 测试PrimVolDesc */
		ColorPrintf(WHITE, "正在测试PrimVolDesc\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcPVD = JumpToISOPrimVolDesc(mapFile)),//跳转到PrimVolDesc
			JumpToISOPrimVolDesc跳转到NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestPrimVolDesc(pcPVD) != ISO_PROCESS_SUCCESS,
			TestPrimVolDesc检测失败)
		ColorPrintf(GREEN, "通过\n");

		/*! 测试BootRecordVolDesc */
		ColorPrintf(WHITE, "正在测试BootRecordVolDesc\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcBRVD = JumpToISOBootRecordVolDesc(mapFile, pcPVD)),//跳转到BootRecordVolDesc
			JumpToISOBootRecordVolDesc跳转到NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestBootRecordVolDesc(pcBRVD) != ISO_PROCESS_SUCCESS,
			TestBootRecordVolDesc检测失败)
		ColorPrintf(GREEN, "通过\n");

		/*! 测试ValidationEntry */
		ColorPrintf(WHITE, "正在测试ValidationEntry\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcVE = JumpToISOValidationEntry(mapFile, pcBRVD)),//跳转到ValidationEntry
			JumpToISOValidationEntry跳转到NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestValidationEntry(pcVE) != ISO_PROCESS_SUCCESS,
			TestValidationEntry检测失败)
		ColorPrintf(GREEN, "通过\n");

		/*! 测试InitialEntry */
		ColorPrintf(WHITE, "正在测试InitialEntry\t\t");
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			!(pcIE = JumpToISOInitialEntry(mapFile, pcVE)),//跳转到InitialEntry
			JumpToISOInitialEntry跳转到NULL)
		TEST_IF_TRUE_SET_ERR_AND_BREAK(
			TestInitialEntry(pcIE) != ISO_PROCESS_SUCCESS,
			TestInitialEntry检测失败)
		ColorPrintf(GREEN, "通过\n");

		/*! 测试完毕 */
		ColorPrintf(GREEN, "全部测试通过！\n");
	}while(0);

	if(errStrPtr)//测试有错误发生
	{
		ColorPrintf(RED, "%s\n", errStrPtr);
	}

#undef TEST_IF_TRUE_SET_ERR_AND_BREAK
}

#endif

//========================测试代码结束=============================
