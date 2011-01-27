/*!
\file MapFile.c
\author LiuBao
\version 1.0
\date 2010/12/28
\brief 介质操作函数实现
*/

#define WINVER 0x0500    ///< 为了使用GetFileSizeEx

#include <Windows.h>     /* 使用文件映射 */
#include <io.h>          /* _access，检测文件存在 */
#include <assert.h>
#include "ProjDef.h"
#include "MapFile.h"
#include "SafeMemory.h"

#define T media_t              ///< 为抽象数据类型T定义实际名

struct T                       ///  抽象数据类型media_t定义
{
    const _TCHAR *name;        ///< 打开的介质名（路径）
    HANDLE hFile;              ///< 映射介质时使用的句柄
    FILE *pFile;               ///< 直接IO时使用的结构体
    PVOID pView;               ///< 映射介质时，视图指针
    uint32_t allocGran;        ///< 内存分配粒度
    uint32_t viewSize;         ///< 视图大小，映射时使用VIEW，直接IO时使用重定向缓冲区大小
    uint32_t actualViewSize;   ///< 实际视图大小，实际视图大小根据不同情况可能小于等于viewSize
    uint32_t accessSize;       ///< 可访问视图大小，即从currPos到视图末尾的大小
    LARGE_INTEGER size;        ///< 介质大小
    LARGE_INTEGER viewPos;     ///< 当前视图所在位置，相对介质起始位置偏移量
    LARGE_INTEGER currPos;     ///< 当前所在位置，相对介质起始位置偏移量
};

static HANDLE MapMedia(const _TCHAR *path, PLARGE_INTEGER lpFileSize);
static int SplitMapView(T media, int64_t offset);
static int FullMapView(T media, uint32_t offset);
static int SeekMapMedia(T media, int64_t offset, int base);
static int SeekRawMedia(T media, int64_t offset, int base);
static T OpenMapMedia(T media, const _TCHAR *path, uint32_t viewSize);

static HANDLE MapMedia(const _TCHAR *path, PLARGE_INTEGER lpFileSize)
{
    HANDLE hFileMapping = NULL;

    if(path && lpFileSize)
    {
        HANDLE hFile = CreateFile(
            path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(hFile != INVALID_HANDLE_VALUE)
        {
            if(GetFileSizeEx(hFile, lpFileSize))//获得文件大小
            {
                hFileMapping = CreateFileMapping(
                    hFile, NULL, PAGE_READONLY, lpFileSize->HighPart, lpFileSize->LowPart, NULL);
            }
            CloseHandle(hFile);
        }
    }

    return hFileMapping;
}

static int SplitMapView(T media, int64_t offset)
{
#define ALIGN_COST (offset & (media->allocGran - 1)) //对齐消耗的大小

    int retVal = FAILED;

    LARGE_INTEGER alignedOffset;

    assert((offset & ~(media->allocGran - 1)) == (offset / media->allocGran) * media->allocGran);

    /* 偏移量与内存粒度对齐 */
    alignedOffset.QuadPart = offset & ~(media->allocGran - 1);

    if(media->pView && alignedOffset.QuadPart == media->viewPos.QuadPart)
    {
        media->currPos.QuadPart = offset;
        media->accessSize = media->actualViewSize - ALIGN_COST;

        return SUCCESS;
    }
    else//如果对齐后偏移量与当前视图位置不同，或者还未映射
    {
        /*
            每当跳转位置offset超过当前视图位置viewPos后
            一个内存分配粒度allocGran时，offset对齐位置
            alignedOffset将与跳转前的视图位置viewPos不同，
            即会执行此段else代码。

            此段else代码在新位置alignedOffset映射新视图，
            成功后销毁原来视图并更新media结构值。
        */
        uint32_t actualViewSize;
        PVOID pNewView;

        /* 计算实际视图大小actualViewSize */
        if(alignedOffset.QuadPart + media->viewSize > media->size.QuadPart)
            actualViewSize = (uint32_t)(media->size.QuadPart - alignedOffset.QuadPart);
        else
            actualViewSize = media->viewSize;

        pNewView = MapViewOfFile(
            media->hFile, FILE_MAP_READ, alignedOffset.HighPart, alignedOffset.LowPart, actualViewSize);

        if(pNewView)
        {
            /* 若media存在旧映射视图,并且销毁成功,或者media不存在旧映射视图 */
            if((media->pView && UnmapViewOfFile(media->pView)) || !media->pView)
            {
                media->pView = pNewView;
                media->viewPos = alignedOffset;
                media->actualViewSize = actualViewSize;

                /*
                    可访问视图大小应当等于实际的视图大小actualViewSize减去
                    对齐offset时消耗的大小，即offset-alignedOffset，
                    这里优化一下使用位运算提高速度。
                */
                media->currPos.QuadPart = offset;
                media->accessSize = actualViewSize - ALIGN_COST;

                retVal = SUCCESS;
            }
        }
    }

    return retVal;

#undef ALIGN_COST
}

static int FullMapView(T media, uint32_t offset)
{
    int retVal = FAILED;

    if(media->pView)//如果已经映射成功
    {
        /*
            offset已经是绝对偏移量，这里可访问视图大小accessSize
            直接是实际视图大小（同样也是介质大小）减去offset即可
        */
        media->currPos.QuadPart = offset;
        media->accessSize = media->actualViewSize - offset;

        retVal = SUCCESS;
    }
    else//如果还未映射，则进行完全映射
    {
        PVOID pNewView = MapViewOfFile(
            media->hFile, FILE_MAP_READ, 0, 0, 0);

        if(pNewView)
        {
            media->pView = pNewView;

            /*
                首次（也是唯一一次）进行完全映射后，
                可访问视图大小和实际视图大小应当与介质
                大小相同
            */
            media->accessSize = media->size.LowPart;
            media->actualViewSize = media->size.LowPart;

            retVal = SUCCESS;
        }
    }    

    return retVal;
}

static int SeekMapMedia(T media, int64_t offset, int base)
{
    int retVal = FAILED;

    if(media)
    {
        LARGE_INTEGER mendedOffset;

        /* 修正offset为绝对偏移量 */
        switch(base)
        {
        case MEDIA_SET:
            mendedOffset.QuadPart = offset;
            break;
        case MEDIA_CUR:
            mendedOffset.QuadPart = offset + media->currPos.QuadPart;
            break;
        }

        /* 如果修正后偏移量mendedOffset没有超出文件范围 */
        if(mendedOffset.QuadPart >= 0 && mendedOffset.QuadPart < media->size.QuadPart)
        {
            if(media->viewSize)//如果分块映射
                retVal = SplitMapView(media, mendedOffset.QuadPart);
            else//如果完全映射
                retVal = FullMapView(media, mendedOffset.LowPart);
        }
    }

    return retVal;
}

static int SeekRawMedia(T media, int64_t offset, int base)
{
    int retVal = FAILED;

    if(media && offset)
    {
        assert(0);/// \todo SeekRawMedia中加入使用fopen打开介质
    }

    return retVal;
}

static T OpenMapMedia(T media, const _TCHAR *path, uint32_t viewSize)
{
    T retVal = NULL;

    if(media && path)
    {
        LARGE_INTEGER fileSize;
        HANDLE hFileMapping = MapMedia(path, &fileSize);

        if(hFileMapping)
        {
            SYSTEM_INFO sysInfo;
            
            /* 获取内存分配粒度 */
            GetSystemInfo(&sysInfo);

            if(sysInfo.dwAllocationGranularity)
            {
                uint32_t roundViewSize = /* 上对齐与内存粒度 */
                    (viewSize & ~(sysInfo.dwAllocationGranularity - 1)) + sysInfo.dwAllocationGranularity;
                    
                assert(((viewSize / sysInfo.dwAllocationGranularity) + 1) * sysInfo.dwAllocationGranularity
                    == roundViewSize);

                if(roundViewSize >= viewSize)//防止上对齐整溢出
                {
                    viewSize = roundViewSize;

                    if(!fileSize.HighPart && viewSize >= fileSize.LowPart)//viewSize大于fileSize
                        viewSize = 0;

                    /* 初始化media结构体 */
                    media->name = path;
                    media->hFile = hFileMapping;
                    media->allocGran = sysInfo.dwAllocationGranularity;
                    media->size = fileSize;
                    media->viewSize = viewSize;
                    media->accessSize = 0;
                    media->actualViewSize = 0;
                    media->viewPos.QuadPart = 0;
                    media->currPos.QuadPart = 0;
                    media->pFile = NULL;
                    media->pView = NULL;

                    if(RewindMedia(media) == SUCCESS)
                        retVal = media;
                }
            }
        }
    }

    return retVal;
}

int RewindMedia(T media)
{
    return SeekMedia(media, 0, MEDIA_SET);
}

int SeekMedia(T media, int64_t offset, int base)
{
    int retVal = FAILED;

    if(media && (base == MEDIA_SET || base == MEDIA_CUR))
    {
        if(media->hFile)//如果是映射文件
            retVal = SeekMapMedia(media, offset, base);
        else if(media->pFile)//如果是fopen打开介质
            retVal = SeekRawMedia(media, offset, base);
    }

    return retVal;
}

T OpenMedia(const _TCHAR *path, uint32_t viewSize)
{
    T media, retVal = NULL;

    if(path && NEW(media))
    {
        if(_taccess(path, 0/* 检测存在 */))//如果文件不存在
        {
            assert(0);/// \todo 添加文件不存在的处理
        }
        else//如果文件存在
            retVal = OpenMapMedia(media, path, viewSize);
    }

    return retVal;
}

void CloseMedia(T *media)
{
    T mediaCopy = *media;

    if(media && mediaCopy)
    {
        if(mediaCopy->hFile)//如果是映射文件
        {
            UnmapViewOfFile(mediaCopy->pView);   /* 卸载视图 */
            CloseHandle(mediaCopy->hFile);       /* 关闭文件句柄 */
            FREE(mediaCopy);                     /* 释放media_t结构 */
            *media = NULL;                       /* 参数指向的media_t置NULL */
        }
        else if(mediaCopy->pFile)//如果是fopen打开介质
        {
            assert(0);
        }
    }
}

int GetMediaAccess(T media, media_access *access, uint32_t len)
{
    int retVal = FAILED;

    if(media && access && len)
    {
        if(media->hFile)
        {
            if(len <= media->accessSize)
            {
                access->begin = (unsigned char *)media->pView + media->viewSize - media->accessSize;
                access->len = len;
                retVal = SUCCESS;
            }
            else
            {
                assert(0);/// \todo 添加动态分配内存空间
            }
        }
        else if(media->pFile)
        {
            assert(0);
        }
    }
    
    return retVal;
}

int DumpMedia(T media, FILE *fp, int64_t size)
{
    int retVal = FAILED;

    if(media && fp && size)
    {
        LARGE_INTEGER currPos = media->currPos;

#ifdef _DEBUG
        media_t media_bak;    
        assert(NEW(media_bak));
        memcpy(media_bak, media, sizeof(*media));
#endif

        if(media->hFile)//如果是文件映射
        {
            int hasError = 0;

            while(size > media->accessSize)
            {
                const unsigned char *writePtr = 
                    (const unsigned char *)media->pView + media->viewSize - media->accessSize;
                uint32_t writeLen;//写入长度

                if(fwrite(writePtr, media->accessSize, 1, fp) != 1)
                {
                    hasError = 1;
                    break;
                }

                writeLen = media->accessSize;//保存写入的长度
                
                /* 向后跳转，会修改media->accessSize */
                if(SeekMedia(media, media->accessSize, MEDIA_CUR) != SUCCESS)
                {
                    hasError = 1;
                    break;
                }

                size -= writeLen;
            }

            if(!hasError)
            {
                if(size && fwrite(media->pView, (size_t)size, 1, fp) == 1)
                    retVal = SUCCESS;
            }
        }
        else if(media->pFile)
        {
            assert(0);/// \todo 添加fopen打开的介质处理
        }

        if(SeekMedia(media, currPos.QuadPart, MEDIA_SET) != SUCCESS)
            retVal = FAILED;

#ifdef _DEBUG
        assert(!memcmp(media_bak, media, sizeof(*media)));
#endif
    }

    return retVal;
}

//========================测试代码开始=============================

#ifdef _DEBUG

#include "ColorPrint.h"    /*!< 测试用例使用 */

#ifdef _UNICODE
#define _tmemcmp wmemcmp
#else
#define _tmemcmp memcmp
#endif

void MapTestUnit(const _TCHAR *path)
{
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

    LARGE_INTEGER fileSize;
    uint32_t allocGran;

    fileSize.QuadPart = 0;//初始设0
    allocGran = 0;

    /* 获得文件大小fileSize */
    {
        HANDLE hFile = CreateFile(
            path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(hFile != INVALID_HANDLE_VALUE)
            GetFileSizeEx(hFile, &fileSize);//获得文件大小

        CloseHandle(hFile);
    }

    /* 获得内存分配粒度大小allocGran */
    {
        SYSTEM_INFO sysInfo;

        /* 获取内存分配粒度 */
        GetSystemInfo(&sysInfo);
        allocGran = sysInfo.dwAllocationGranularity;
    }


    {
        uint32_t viewSize;

        ColorPrintf(WHITE, _T("正在测试文件打开\t\t\t"));
        assert(fileSize.QuadPart);
        ColorPrintf(GREEN, _T("通过\n"));

        ColorPrintf(WHITE, _T("正在测试获取内存分配粒度\t\t"));
        assert(allocGran);
        ColorPrintf(GREEN, _T("通过\n"));

        ColorPrintf(YELLOW, _T("=>文件大小：%I64d\t内存分配粒度：\%u\n"), fileSize.QuadPart, allocGran);

        ColorPrintf(SILVER, _T("->取视图 >= 文件，将进行完全映射：\n"));
        {
            LARGE_INTEGER tempViewSize;

            tempViewSize.QuadPart = MAX(fileSize.QuadPart, allocGran) + 10;

            if(tempViewSize.HighPart)
                ColorPrintf(RED, _T("文件大小超过视图上限，跳过检测！\n"));
            else
            {
                T testMedia, backupMedia = NULL;

                viewSize = tempViewSize.LowPart;

                ColorPrintf(WHITE, _T("正在测试映射文件\t\t\t"));
                testMedia = OpenMedia(path, viewSize);
                assert(testMedia->allocGran == allocGran);                   /* 检查粒度正确 */
                assert(!testMedia->currPos.QuadPart);                        /* 检查当前位置为0 */
                assert(testMedia->hFile);                                    /* 检查文件句柄存在 */
                assert(!_tmemcmp(testMedia->name, path, _tcslen(path) + 1)); /* 检查名称正确 */
                assert(testMedia->pView);                                    /* 检查视图指针应当不为空 */
                assert(!testMedia->pFile);                                   /* 检查文件指针应当为空 */
                assert(testMedia->size.QuadPart == fileSize.QuadPart);       /* 检查文件大小正确 */
                assert(testMedia->accessSize);                               /* 检查可访问视图大小应当不为0 */
                assert(!testMedia->viewPos.QuadPart);                        /* 检查视图起始位置为0 */
                assert(!testMedia->viewSize);                                /* 检查视图大小应当为0 */
                assert(!(testMedia->viewPos.QuadPart % allocGran));          /* 检查视图起始位置对齐内存粒度 */
                ColorPrintf(GREEN, _T("通过\n"));

                ColorPrintf(WHITE, _T("正在测试跳转\t\t\t\t"));
                assert(NEW(backupMedia));                                                                         /*!< 创建backupMedia */
                assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));                                       /*!< 备份media到backupMedia */
                assert(SeekMedia(testMedia, testMedia->accessSize, MEDIA_SET) != SUCCESS);                        /*!< 测试跳转到可访问范围边界，应当失败 */
                assert(!memcmp(backupMedia, testMedia, sizeof(*testMedia)));                                      /*!< 跳转失败时，media值应当不变 */
                assert(SeekMedia(testMedia, testMedia->accessSize - 1, MEDIA_SET) == SUCCESS);                    /*!< 测试跳转到可访问范围边界-1，应当成功 */
                assert(RewindMedia(testMedia) == SUCCESS);                                                        /*!< 重置media */
                assert(!memcmp(backupMedia, testMedia, sizeof(*testMedia)));                                      /*!< 检查重置后media应当与原始media相同 */
                assert(SeekMedia(testMedia, testMedia->accessSize - 1, MEDIA_SET) == SUCCESS);                    /*!< 测试跳转到可访问范围边界-1，应当成功 */
                assert(SeekMedia(testMedia, -(int64_t)(testMedia->actualViewSize - 1), MEDIA_CUR) == SUCCESS);    /*!< 从当前向后跳回同样长度 */
                assert(!memcmp(backupMedia, testMedia, sizeof(*testMedia)));                                      /*!< 检查重置后media应当与原始media相同 */
                assert(SeekMedia(testMedia, testMedia->accessSize - 1, MEDIA_SET) == SUCCESS);                    /*!< 测试跳转到可访问范围边界-1，应当成功 */
                assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));                                       /*!< 备份当前media到backupMedia */
                assert(SeekMedia(testMedia, -(int64_t)(testMedia->actualViewSize - 1)/2, MEDIA_CUR) == SUCCESS);  /*!< 向后跳回（可访问范围边界-1）/2 */
                assert(SeekMedia(testMedia, (int64_t)(testMedia->actualViewSize - 1)/2, MEDIA_CUR) == SUCCESS);   /*!< 向前跳回（可访问范围边界-1）/2 */
                assert(!memcmp(backupMedia, testMedia, sizeof(*testMedia)));                                      /*!< 检查跳回后media应当与上次备份的backupMedia相同 */
                ColorPrintf(GREEN, _T("通过\n"));

                CloseMedia(&testMedia);
            }
        }

        ColorPrintf(SILVER, _T("->取视图 < 文件，将进行分块映射：\n"));
        {
            LARGE_INTEGER tempViewSize;

            tempViewSize.QuadPart = fileSize.QuadPart - allocGran - 1;

            if(tempViewSize.HighPart)
                ColorPrintf(RED, _T("文件大小超过视图上限，跳过检测！\n"));
            else if(tempViewSize.QuadPart <= allocGran)
                ColorPrintf(RED, _T("文件大小接近内存分配粒度，跳过检测！\n"));
            else if((tempViewSize.LowPart & ~(allocGran - 1)) + allocGran >= fileSize.QuadPart)
                ColorPrintf(RED, _T("视图大小上对齐内存分配粒度后 >= 文件大小，跳过检测！\n"));
            else
            {
                T testMedia, backupMedia = NULL;

                viewSize = tempViewSize.LowPart;

                ColorPrintf(WHITE, _T("正在测试映射文件\t\t\t"));
                testMedia = OpenMedia(path, viewSize);
                assert(testMedia->allocGran == allocGran);                   /* 检查粒度正确 */
                assert(!testMedia->currPos.QuadPart);                        /* 检查当前位置为0 */
                assert(testMedia->hFile);                                    /* 检查文件句柄存在 */
                assert(!_tmemcmp(testMedia->name, path, _tcslen(path) + 1)); /* 检查名称正确 */
                assert(testMedia->pView);                                    /* 检查视图指针应当不为空 */
                assert(!testMedia->pFile);                                   /* 检查文件指针应当为空 */
                assert(testMedia->size.QuadPart == fileSize.QuadPart);       /* 检查文件大小正确 */
                assert(testMedia->accessSize);                               /* 检查可访问视图大小应当不为0 */
                assert(!testMedia->viewPos.QuadPart);                        /* 检查视图起始位置为0 */
                assert(testMedia->viewSize);                                 /* 检查视图大小应当不为0 */
                assert(!(testMedia->viewPos.QuadPart % allocGran));          /* 检查视图起始位置对齐内存粒度 */
                assert(testMedia->viewSize >= viewSize);                     /* 与粒度上对齐后，实际的viewSize应当大于等于传入viewSize */
                ColorPrintf(GREEN, _T("通过\n"));

                ColorPrintf(WHITE, _T("正在测试跳转\t\t\t\t"));
                assert(NEW(backupMedia));                                                                  /*!< 创建backupMedia */
                assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));                                /*!< 备份media到backupMedia */
                assert(SeekMedia(testMedia, testMedia->allocGran - 1, MEDIA_SET) == SUCCESS);              /*!< 局部跳转，应该不会切换映射视图 */
                assert(testMedia->pView == backupMedia->pView);                                            /*!< 检查局部跳转后视图仍然是原来视图 */
                assert(testMedia->viewPos.QuadPart == backupMedia->viewPos.QuadPart);                      /*!< 检查局部跳转后视图位置仍然是原来位置 */
                assert(testMedia->currPos.QuadPart != testMedia->viewPos.QuadPart);                        /*!< 检查局部跳转后当前位置应当与视图位置不同 */
                assert(RewindMedia(testMedia) == SUCCESS);                                                 /*!< 重置media */
                assert(!memcmp(testMedia, backupMedia, sizeof(*testMedia)));                               /*!< 检查重置后的media与以前备份的相同 */
                assert(SeekMedia(testMedia, testMedia->allocGran, MEDIA_SET) == SUCCESS);                  /*!< 跳转一个内存粒度，应该刚好切换视图 */
                assert(testMedia->pView != backupMedia->pView);                                            /*!< 检查跳转后视图是否已切换 */
                assert(testMedia->currPos.QuadPart == testMedia->viewPos.QuadPart);                        /*!< 由于跳转位置刚好切换视图，跳转后当前位置应当与视图位置相同 */
                assert(testMedia->accessSize == testMedia->actualViewSize);                                /*!< 由于跳转位置刚好切换视图，跳转后可访问大小正好是实际视图大小 */
                assert(testMedia->viewSize == backupMedia->viewSize);                                      /*!< 跳转后视图大小应当不变 */
                assert(testMedia->actualViewSize < backupMedia->actualViewSize);                           /*!< 由于跳转位置距离文件末尾长度小于视图大小，跳转后实际视图大小应当小于视图大小 */
                assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));                                /*!< 保存跳转后的media到backupMedia */
                assert(SeekMedia(testMedia, -1, MEDIA_CUR) == SUCCESS);                                    /*!< 向后跳转1，应该切换视图 */
                assert(testMedia->pView != backupMedia->pView);                                            /*!< 检查跳转后视图是否已切换 */
                assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));                                /*!< 保存跳转后的media到backupMedia */
                assert(SeekMedia(testMedia, -(int64_t)(testMedia->allocGran - 1), MEDIA_CUR) == SUCCESS);  /*!< 这时从当前位置向后跳转一个内存粒度 */
                assert(testMedia->viewPos.QuadPart == 0);                                                  /*!< 这时相当于RewindMedia后，视图位置应当为0 */
                assert(testMedia->currPos.QuadPart == 0);                                                  /*!< 这时相当于RewindMedia后，当前位置应当为0 */
                assert(testMedia->actualViewSize == testMedia->viewSize);                                  /*!< 实际视图大小应当与视图大小相同 */
                assert(SeekMedia(testMedia, testMedia->allocGran - 1, MEDIA_SET) == SUCCESS);              /*!< 再向前跳回一个内存粒度 */
                assert(!memcmp(testMedia, backupMedia, sizeof(*testMedia)));                               /*!< 检查media与上次备份的backupMeida相同 */
                ColorPrintf(GREEN, _T("通过\n"));

                CloseMedia(&testMedia);
            }
        }

        /* 测试完毕 */
        ColorPrintf(GREEN, _T("全部测试通过！\n"));
    }

#undef MIN
#undef MAX
}

#endif

//========================测试代码结束=============================