/*!
\file MapFile_Test.c
\author LiuBao
\version 1.0
\date 2011/1/29
\brief MapFile的单元测试
*/

#define WINVER 0x0500       ///< 为了使用GetFileSizeEx

#include <limits.h>
#include <assert.h>
#include <tchar.h>
#include <Windows.h>
#include "MapFile_Test.h"
#include "..\ProjDef.h"     /* 使用SUCCESS和FAILED */
#include "..\SafeMemory.h"
#include "..\MapFile.h"
#include "..\AprRing\apr_ring.h"

#define MAP_FILE_PATH _T(".\\data\\MapFile.file")  ///< 生成用于测试的映射文件路径
#define OUT_PUT_PATH _T(".\\data\\Output.file")    ///< 输出文件路径

typedef int MapFileType;                    ///< 映射文件类型

#define LARGER_THAN_ALLOC_GRAN 0x1          ///< 大于内存分配粒度
#define SMALLER_THAN_ALLOC_GRAN 0x2         ///< 小于内存分配粒度
#define EQUAL_TO_ALLOC_GRAN 0x3             ///< 等于内存分配粒度

struct elem_t                               ///< APR环元素类型定义
{
    APR_RING_ENTRY(elem_t) link;            ///< 链接域
    media_access *access;                   ///< 数据域
};

APR_RING_HEAD(elem_head_t, elem_t);         ///< APR环头结点定义

#define T hack_t                            ///< 为抽象数据类型T定义实际名

enum MediaType                              ///< 介质类型
{
    Map,                                    ///< 文件映射
    Raw                                     ///< 直接IO
};

struct T                                    ///  抽象数据类型media_t定义
{
    union                                   ///  哑联合
    {
        struct _map                         ///  文件映射数据结构
        {
            const _TCHAR *name;             ///< 打开的介质名（路径）
            HANDLE hFile;                   ///< 文件映射句柄
            PVOID pView;                    ///< 映射视图指针
            uint32_t allocGran;             ///< 内存分配粒度
            uint32_t viewSize;              ///< 视图大小，映射时使用VIEW，直接IO时使用重定向缓冲区大小
            uint32_t actualViewSize;        ///< 实际视图大小，实际视图大小根据不同情况可能小于等于viewSize
            uint32_t accessSize;            ///< 可访问视图大小，即从currPos到视图末尾的大小
            LARGE_INTEGER size;             ///< 介质大小
            LARGE_INTEGER viewPos;          ///< 当前视图所在位置，相对介质起始位置偏移量
            LARGE_INTEGER currPos;          ///< 当前所在位置，相对介质起始位置偏移量
            struct elem_head_t accessRing;  ///< media_access指针（环状）链表头结点
        }map;

        struct _raw                         ///  直接IO数据结构
        {
            const _TCHAR *name;             ///< 打开的介质名（路径）
            HANDLE hFile;                   ///< 介质句柄
            PVOID pBuffer;                  ///< 缓冲区指针
            uint32_t sectorSize;            ///< 介质扇区大小
            uint32_t accessSize;            ///< 可访问视图大小，即从currPos到视图末尾的大小
            LARGE_INTEGER size;             ///< 介质大小
            LARGE_INTEGER currPos;          ///< 当前所在位置，相对介质起始位置偏移量
        }raw;
    };

    enum MediaType type;
};

typedef struct T *T;

/*!
根据文件路径获取文件大小
\param path 文件路径
\param lpSize 文件大小指针
\return 获取文件大小成功，返回SUCCESS；否则返回FAILED
*/
static int FileSize(const _TCHAR *path, PLARGE_INTEGER lpSize)
{
    int retVal = FAILED;

    if(path && lpSize)
    {
        HANDLE hFile = CreateFile(
            path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(hFile != INVALID_HANDLE_VALUE)
        {
            GetFileSizeEx(hFile, lpSize);//获得文件大小
            retVal = SUCCESS;
        }

        CloseHandle(hFile);
    }

    return retVal;
}

/*!
获取内存分配粒度
\param lpAllocGran 内存分配粒度大小指针
\return 获取内存分配粒度成功，返回SUCCESS；否则返回FAILED
*/
static int AllocGran(uint32_t *lpAllocGran)
{
    int retVal = FAILED;

    if(lpAllocGran)
    {
        SYSTEM_INFO sysInfo;

        /* 获取内存分配粒度 */
        GetSystemInfo(&sysInfo);
        *lpAllocGran = sysInfo.dwAllocationGranularity;
        retVal = SUCCESS;
    }

    return retVal;
}

/*!
根据内存分配粒度及传入参数在path生成测试文件
\param path 测试文件路径
\param allocGran 内存分配粒度
\param type 测试文件类型
\return 生成测试文件成功，返回SUCCESS；否则返回FAILED
*/
static int CreateMapFile(const _TCHAR *path, uint32_t allocGran, MapFileType type)
{
    int retVal = FAILED;

    if(path && allocGran)
    {
        FILE *fp = _tfopen(path, _T("wb"));

        if(fp)
        {
            switch(type)
            {
            case LARGER_THAN_ALLOC_GRAN://比内存分配粒度大
                {
                    uint32_t i;

                    assert(UINT_MAX != allocGran);

                    if(UINT_MAX - allocGran >= 10)
                    {
                        for(i = 0; i < allocGran + 10; ++i)
                            if(fputc(0x11, fp) == EOF)
                                break;

                        if(i == allocGran + 10)
                            retVal = SUCCESS;
                    }
                }
                break;
            case SMALLER_THAN_ALLOC_GRAN://比内存分配粒度小
                {
                    uint32_t i;

                    if(allocGran > 10)
                    {
                        for(i = 0; i < allocGran - 10; ++i)
                            if(fputc(0x22, fp) == EOF)
                                break;

                        if(i == allocGran - 10)
                            retVal = SUCCESS;
                    }
                }
                break;
            case EQUAL_TO_ALLOC_GRAN://与内存分配粒度大小相同
                {
                    uint32_t i;

                    for(i = 0; i < allocGran; ++i)
                        if(fputc(0x33, fp) == EOF)
                            break;

                    if(i == allocGran)
                        retVal = SUCCESS;
                }
                break;
            default:
                break;
            }
        }

        fclose(fp);
    }

    return retVal;
}

static void TestMapNotExistFile(lcut_tc_t *tc, void *data)
{
     LCUT_ASSERT(tc, "Open not exist file should return NULL", OpenMedia(_T(".\\data\\NotExistFile"), 1) == NULL);
}

static void TestMapFileOpen(lcut_tc_t *tc, void *data)
{
    int status = 0;
    int testFileType = LARGER_THAN_ALLOC_GRAN;

    /* 创建测试文件 */
    do
    {
        uint32_t allocGran = 0;
        LARGE_INTEGER fileSize;
        status = 0;

        if(AllocGran(&allocGran) != SUCCESS)
            break;

        if(CreateMapFile(MAP_FILE_PATH, allocGran, testFileType) != SUCCESS)
            break;

        if(FileSize(MAP_FILE_PATH, &fileSize) != SUCCESS)
            break;

        /* 检查media_t状态 */
        {
            media_t media = OpenMedia(MAP_FILE_PATH, 0);
            hack_t hack = (hack_t)media;

            LCUT_ASSERT(tc, "OpenMedia返回NULL", hack != NULL);
            LCUT_ASSERT(tc, "文件映射时，media_t->hFile为INVALID_HANDLE_VALUE", hack->map.hFile != INVALID_HANDLE_VALUE);
            LCUT_ASSERT(tc, "文件映射时，media_t->pView为NULL", hack->map.pView != NULL);
            LCUT_ASSERT(tc, "media_t中内存分配粒度错误", hack->map.allocGran == allocGran);
            LCUT_ASSERT(tc, "media_t中文件大小错误", hack->map.size.QuadPart == fileSize.QuadPart);
            LCUT_ASSERT(tc, "media_t中可访问视图大小为0", hack->map.accessSize != 0);
            LCUT_ASSERT(tc, "media_t中当前位置不为0", hack->map.currPos.QuadPart == 0);
            LCUT_ASSERT(tc, "media_t中当前视图起始位置不为0", hack->map.viewPos.QuadPart == 0);

            if(testFileType == LARGER_THAN_ALLOC_GRAN)
                /* 文件大于内存分配粒度，使用分块映射，此时viewSize应当至少为1个allocGran */
                LCUT_ASSERT(tc, "media_t->viewSize错误", hack->map.viewSize >= allocGran);
            else
                /* 文件小于等于内存分配粒度，使用完全映射，此时viewSize为0 */
                LCUT_ASSERT(tc, "media_t->viewSize不为0", hack->map.viewSize == 0);

            CloseMedia(&media);
        }

        status = 1;
    }while(++testFileType <= 3);

    /* 断言测试文件创建成功 */
    LCUT_ASSERT(tc, "测试文件创建失败", status);
}

static void TestMapFileJumpWithFullMap(lcut_tc_t *tc, void *data)
{
    uint32_t allocGran = 0;

    if(AllocGran(&allocGran) == SUCCESS && 
        CreateMapFile(MAP_FILE_PATH, allocGran, SMALLER_THAN_ALLOC_GRAN) == SUCCESS)
    {
        media_t testMedia = OpenMedia(MAP_FILE_PATH, 0);
        hack_t backupMedia;

        LCUT_ASSERT(tc, "打开media_t失败", testMedia != NULL);
        LCUT_ASSERT(tc, "创建备份media_t失败", NEW(backupMedia) != NULL);

        /* 备份testMedia到backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* 测试跳转到可访问范围边界，应当失败 */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.accessSize, MEDIA_SET) != SUCCESS);

        /* 测试向后跳转1，应当失败 */
        LCUT_TRUE(tc, SeekMedia(testMedia, -1, MEDIA_SET) != SUCCESS);

        /* 跳转失败时，media_t状态应当不变 */
        LCUT_TRUE(tc, !memcmp(backupMedia, testMedia, sizeof(*backupMedia)));

        /* 测试跳转到可访问范围边界-1，应当成功 */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.accessSize - 1, MEDIA_SET) == SUCCESS);

        /* 重置media */
        LCUT_TRUE(tc, RewindMedia(testMedia) == SUCCESS);

        /* 检查重置后media_t应当与原始media_t备份（backupMedia）相同 */
        LCUT_TRUE(tc, !memcmp(backupMedia, testMedia, sizeof(*backupMedia)));

        /* 再次跳转到可访问范围边界-1，应当成功 */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.accessSize - 1, MEDIA_SET) == SUCCESS);

        /* 测试向后跳转（可访问范围边界-1）距离 */
        LCUT_TRUE(tc, SeekMedia(testMedia, -(int64_t)(backupMedia->map.actualViewSize - 1), MEDIA_CUR) == SUCCESS);
    
        /* 此时应跳回初始位置，media_t应当与原始media_t备份（backupMedia）相同 */
        LCUT_TRUE(tc, !memcmp(backupMedia, testMedia, sizeof(*backupMedia)));

        /* 再次跳转到可访问范围边界-1，应当成功 */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.accessSize - 1, MEDIA_SET) == SUCCESS);

        /* 备份当前状态的testMedia到backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* 测试向后跳转到可访问范围边界，应当失败 */
        LCUT_TRUE(tc, SeekMedia(testMedia, -(int64_t)(backupMedia->map.actualViewSize), MEDIA_CUR) != SUCCESS);

        /* 向后跳回（可访问范围边界-1）/2 */
        LCUT_TRUE(tc, SeekMedia(testMedia, -(int64_t)(backupMedia->map.actualViewSize - 1)/2, MEDIA_CUR) == SUCCESS);

        /* 向前跳回（可访问范围边界-1）/2 */
        LCUT_TRUE(tc, SeekMedia(testMedia, (int64_t)(backupMedia->map.actualViewSize - 1)/2, MEDIA_CUR) == SUCCESS);
    
        /* 检查跳转后media_t应当与media_t备份（backupMedia）相同 */
        LCUT_TRUE(tc, !memcmp(backupMedia, testMedia, sizeof(*backupMedia)));

        CloseMedia(&testMedia);
    }
}

static void TestMapFileJumpWithSplitMap(lcut_tc_t *tc, void *data)
{
    uint32_t allocGran = 0;

    if(AllocGran(&allocGran) == SUCCESS && 
        CreateMapFile(MAP_FILE_PATH, allocGran, LARGER_THAN_ALLOC_GRAN) == SUCCESS)
    {
        media_t testMedia = OpenMedia(MAP_FILE_PATH, 0);
        hack_t backupMedia;

        LCUT_ASSERT(tc, "打开media_t失败", testMedia != NULL);
        LCUT_ASSERT(tc, "创建备份media_t失败", NEW(backupMedia) != NULL);

        /* 备份testMedia到backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* 局部跳转，应该不会切换映射视图 */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.allocGran - 1, MEDIA_SET) == SUCCESS);

        /* 检查局部跳转后视图仍然是原来视图 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.pView == backupMedia->map.pView);

        /* 检查局部跳转后视图位置仍然是原来位置 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.viewPos.QuadPart == backupMedia->map.viewPos.QuadPart);

        /* 检查局部跳转后当前位置应当与视图位置不同 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.currPos.QuadPart != ((hack_t)testMedia)->map.viewPos.QuadPart);

        /* 重置media */
        LCUT_TRUE(tc, RewindMedia(testMedia) == SUCCESS);

        /* 检查重置后media_t应当与原始media_t备份（backupMedia）相同 */
        LCUT_TRUE(tc, !memcmp(testMedia, backupMedia, sizeof(*backupMedia)));

        /* 跳转一个内存粒度，应该刚好切换视图 */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.allocGran, MEDIA_SET) == SUCCESS);

        /* 检查跳转后视图是否已切换 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.pView != backupMedia->map.pView);

        /* 由于跳转位置刚好切换视图，跳转后当前位置应当与视图位置相同 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.currPos.QuadPart == ((hack_t)testMedia)->map.viewPos.QuadPart);

        /* 由于跳转位置刚好切换视图，跳转后可访问大小正好是实际视图大小 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.accessSize == ((hack_t)testMedia)->map.actualViewSize);

        /* 跳转后视图大小应当不变 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.viewSize == backupMedia->map.viewSize);

        /* 由于跳转位置距离文件末尾长度小于视图大小，跳转后实际视图大小应当小于视图大小 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.actualViewSize < ((hack_t)testMedia)->map.viewSize);

        /* 保存跳转后的media_t到backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* 向后跳转1，应该切换视图 */
        LCUT_TRUE(tc, SeekMedia(testMedia, -1, MEDIA_CUR) == SUCCESS);

        /* 检查跳转后视图是否已切换 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.pView != backupMedia->map.pView);

        /* 保存跳转后的media_t到backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* 这时从当前位置向后跳转(一个内存粒度 - 1) */
        LCUT_TRUE(tc, SeekMedia(testMedia, -(int64_t)(((hack_t)testMedia)->map.allocGran - 1), MEDIA_CUR) == SUCCESS);

        /* 这时相当于RewindMedia后，视图起始位置应当为0 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.viewPos.QuadPart == 0);

        /* 这时相当于RewindMedia后，当前位置应当为0 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.currPos.QuadPart == 0);

        /* 实际视图大小应当与视图大小相同 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.actualViewSize == ((hack_t)testMedia)->map.viewSize);

        /* 再向前跳回一个内存粒度 */
        LCUT_TRUE(tc, SeekMedia(testMedia, ((hack_t)testMedia)->map.allocGran - 1, MEDIA_SET) == SUCCESS);

        /* 检查media与上次备份的backupMeida相同 */
        LCUT_TRUE(tc, !memcmp(testMedia, backupMedia, sizeof(*backupMedia)));

        CloseMedia(&testMedia);
    }
}

static void TestDirectIODevice(lcut_tc_t *tc, void *data)
{
    media_t media = OpenMedia(_T("\\\\.\\D:"), 0);

    //if(media && SeekMedia(media, ((hack_t)media)->size.QuadPart - 2048, MEDIA_CUR) == SUCCESS)
    //{
    //    media_access access;

    //    if(GetMediaAccess(media, &access, 4096) == SUCCESS)
    //        assert(0);
    //}
}

static void TestMapFileAccess(lcut_tc_t *tc, void *data)
{
    uint32_t allocGran = 0;

    if(AllocGran(&allocGran) == SUCCESS && 
        CreateMapFile(MAP_FILE_PATH, allocGran, LARGER_THAN_ALLOC_GRAN) == SUCCESS)
    {
        media_t testMedia = OpenMedia(MAP_FILE_PATH, 0);
        media_access access;
        media_access access2;

        GetMediaAccess(testMedia, &access, ((hack_t)testMedia)->map.size.LowPart);
        GetMediaAccess(testMedia, &access2, ((hack_t)testMedia)->map.size.LowPart - 9);
        SeekMedia(testMedia, 9, MEDIA_CUR);
        GetMediaAccess(testMedia, &access, ((hack_t)testMedia)->map.size.LowPart - 9);
        CloseMedia(&testMedia);
    }
}

static void TestMapFileDump(lcut_tc_t *tc, void *data)
{
    uint32_t allocGran = 0;

    if(AllocGran(&allocGran) == SUCCESS && 
        CreateMapFile(MAP_FILE_PATH, allocGran, SMALLER_THAN_ALLOC_GRAN) == SUCCESS)
    {
        media_t testMedia = OpenMedia(MAP_FILE_PATH, 0);
        FILE *fp = _tfopen(OUT_PUT_PATH, _T("wb"));

        LCUT_ASSERT(tc, "OpenMedia失败", testMedia != NULL);
        LCUT_ASSERT(tc, "fopen失败", fp != NULL);

        SeekMedia(testMedia, 10, MEDIA_CUR);
        DumpMedia(testMedia, fp, ((hack_t)testMedia)->map.size.QuadPart - 10);
        CloseMedia(&testMedia);
    }
}

lcut_ts_t *CreateMapFileTestSuite()
{
    lcut_ts_t *suite = NULL;
    int _cut_status;

    LCUT_TS_INIT(suite, "MapFile 单元测试套件", NULL, NULL);

    LCUT_TC_ADD(suite, "测试文件映射打开文件", TestMapFileOpen, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "测试完全文件映射跳转", TestMapFileJumpWithFullMap, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "测试分块文件映射跳转", TestMapFileJumpWithSplitMap, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "测试直接IO", TestDirectIODevice, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "测试映射文件访问", TestMapFileAccess, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "测试映射文件抽取", TestMapFileDump, NULL, NULL, NULL);

    return suite;
}
