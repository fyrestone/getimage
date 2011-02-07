/*!
\file MapFile.c
\author LiuBao
\version 1.0
\date 2010/12/28
\brief 介质操作函数实现
*/

#define WINVER 0x0500     ///< 为了使用GetFileSizeEx

#include <Windows.h>      /* 使用文件映射 */
#include <io.h>           /* _access，检测文件存在 */
#include <assert.h>
#include <limits.h>       /* 使用INT_MAX */
#include "ProjDef.h"
#include "MapFile.h"
#include "SafeMemory.h"

#define T media_t              ///< 为抽象数据类型T定义实际名

struct T                       ///  抽象数据类型media_t定义
{
    const _TCHAR *name;        ///< 打开的介质名（路径）
    HANDLE hFile;              ///< 当文件映射时存放映射句柄；当直接IO存放介质句柄
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
static HANDLE RawMedia(const _TCHAR *path, PLARGE_INTEGER lpFileSize, uint32_t *lpBlockSize);
static int SplitMapView(T media, int64_t offset);
static int FullMapView(T media, uint32_t offset);
static T OpenMapMedia(T media, const _TCHAR *path, uint32_t viewSize);
static T OpenRawMedia(T media, const _TCHAR *path);
static int MapMediaAccess(T media, media_access *access, uint32_t len);
static int RawMediaAccess(T media, media_access *access, uint32_t len);
static int DumpMapMedia(T media, FILE *fp, int64_t size);
static int DumpRawMedia(T media, FILE *fp, int64_t size);

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

static HANDLE RawMedia(const _TCHAR *path, PLARGE_INTEGER lpFileSize, uint32_t *lpSectorSize)
{
    HANDLE retVal = INVALID_HANDLE_VALUE;

    if(path && lpFileSize && lpSectorSize)
    {
        HANDLE hFile = CreateFile(
            path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

        if(hFile != INVALID_HANDLE_VALUE)
        {
            GET_LENGTH_INFORMATION lenInfo; //介质长度信息结构
            DISK_GEOMETRY geoInfo;          //介质信息
            DWORD readBytes;                //读取到输出缓冲区的的数据大小

            BOOL result;

            /* 直接发送指令获取介质大小 */
            result = DeviceIoControl(hFile, IOCTL_DISK_GET_LENGTH_INFO,
                NULL, 0, &lenInfo, sizeof(GET_LENGTH_INFORMATION), &readBytes, (LPOVERLAPPED)NULL);

            if(result)//若IOCTL_DISK_GET_LENGTH_INFO成功
            {
                /* 写入介质大小 */
                *lpFileSize = lenInfo.Length;

                /* 使用IOCTL_DISK_GET_DRIVE_GEOMETRY获取介质信息 */
                result = DeviceIoControl(hFile, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                    NULL, 0, &geoInfo, sizeof(DISK_GEOMETRY), &readBytes, (LPOVERLAPPED)NULL);

                if(result)//若IOCTL_DISK_GET_DRIVE_GEOMETRY成功
                {
                    /* 写入扇区大小 */
                    *lpSectorSize = geoInfo.BytesPerSector;

                    retVal = hFile;
                }
                else//若IOCTL_DISK_GET_DRIVE_GEOMETRY失败
                {  
                    GET_MEDIA_TYPES *typeInfo = (GET_MEDIA_TYPES *)MALLOC(1024);

                    /* 使用IOCTL_STORAGE_GET_MEDIA_TYPES_EX获取介质信息 */
                    if(typeInfo && DeviceIoControl(hFile, IOCTL_STORAGE_GET_MEDIA_TYPES_EX,
                        NULL, 0, typeInfo, 1024, &readBytes, (LPOVERLAPPED)NULL))
                    {
                        *lpSectorSize = typeInfo->MediaInfo[0].DeviceSpecific.RemovableDiskInfo.BytesPerSector;

                        retVal = hFile;
                    }

                    FREE(typeInfo);
                }
            }

            if(retVal == INVALID_HANDLE_VALUE)//如果上述操作全部失败
                CloseHandle(hFile);
        }
    }

    return retVal;
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

static T OpenMapMedia(T media, const _TCHAR *path, uint32_t viewSize)
{
    T retVal = NULL;

    if(media && path)
    {
        LARGE_INTEGER fileSize;
        HANDLE hFileMapping = MapMedia(path, &fileSize);

        if(hFileMapping != INVALID_HANDLE_VALUE)
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

                    if(!fileSize.HighPart && viewSize >= fileSize.LowPart)//viewSize大于等于fileSize
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
                    media->pView = NULL;

                    if(RewindMedia(media) == SUCCESS)
                        retVal = media;
                }
            }
        }
    }

    return retVal;
}

static T OpenRawMedia(T media, const _TCHAR *path)
{
    T retVal = NULL;

    if(media && path)
    {
        LARGE_INTEGER fileSize;
        uint32_t sectorSize;
        HANDLE hFile = RawMedia(path, &fileSize, &sectorSize);

        if(hFile != INVALID_HANDLE_VALUE)
        {
            /* 初始化media结构体 */
            media->name = path;
            media->hFile = hFile;
            media->allocGran = 0;
            media->size = fileSize;
            media->viewSize = sectorSize;
            media->accessSize = 0;
            media->actualViewSize = 0;
            media->viewPos.QuadPart = 0;
            media->currPos.QuadPart = 0;
            media->pView = NULL;

            retVal = media;
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
            if(media->allocGran)//如果是映射文件
            {
                if(media->viewSize)//如果分块映射
                    retVal = SplitMapView(media, mendedOffset.QuadPart);
                else//如果完全映射
                    retVal = FullMapView(media, mendedOffset.LowPart);
            }
            else//如果是打开介质
            {
                if(media->currPos.QuadPart != mendedOffset.QuadPart)//如果Seek后位置改变
                {
                    media->accessSize = 0;           //可访问大小置0
                    media->currPos = mendedOffset;   //修改当前位置
                }

                retVal = SUCCESS;
            }
        }
    }

    return retVal;
}

T OpenMedia(const _TCHAR *path, uint32_t viewSize)
{
    T media, retVal = NULL;

    if(path && NEW(media))
    {
        if(_taccess(path, 0/* 检测存在 */))//如果文件不存在
            retVal = OpenRawMedia(media, path);
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
        if(mediaCopy->allocGran)//如果是映射文件
        {
            UnmapViewOfFile(mediaCopy->pView);   /* 卸载视图 */
            CloseHandle(mediaCopy->hFile);       /* 关闭文件句柄 */
        }
        else//如果是打开介质
        {
            FREE(mediaCopy->pView);              /* 释放视图 */
            CloseHandle(mediaCopy->hFile);       /* 关闭文件句柄 */
        }

        FREE(mediaCopy);                         /* 释放media_t结构 */
        *media = NULL;                           /* 参数指向的media_t置NULL */
    }
}

static int MapMediaAccess(T media, media_access *access, uint32_t len)
{
    int retVal = FAILED;

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

    return retVal;
}

static int RawMediaAccess(T media, media_access *access, uint32_t len)
{
    int retVal = FAILED;

    /* 切换pView，释放旧pView，申请新pView，并把填写新pView区域 */
    if(len > media->accessSize)//若accessSize为0 或 len > accessSize
    {
        void *tmpView;                  //申请缓冲区空间
        DWORD readBytes;                //读取到输出缓冲区的数据大小
        OVERLAPPED ol;                  //读取偏移量
        LARGE_INTEGER alignedBegin;     //对齐开始偏移量
        LARGE_INTEGER alignedEnd;       //对齐结束偏移量
        uint32_t bufferSize;            //缓冲数据大小

        assert((media->currPos.QuadPart & ~(media->viewSize - 1)) 
            == (media->currPos.QuadPart / media->viewSize) * media->viewSize);

        /* 对齐开始偏移量 */
        alignedBegin.QuadPart = media->currPos.QuadPart & ~(media->viewSize - 1);

        /* 赋值未对齐的结尾位置到alignedEnd */
        alignedEnd.QuadPart = media->currPos.QuadPart + len;

        /* 对齐结束偏移量 */
        if(alignedEnd.QuadPart % media->viewSize)//未对齐
            alignedEnd.QuadPart = (alignedEnd.QuadPart & ~(media->viewSize - 1)) + media->viewSize;

        assert(alignedEnd.QuadPart < media->size.QuadPart);
        assert(alignedEnd.QuadPart - alignedBegin.QuadPart >= 0);
        assert(alignedEnd.QuadPart - alignedBegin.QuadPart <= UINT_MAX);

        bufferSize = (uint32_t)(alignedEnd.QuadPart - alignedBegin.QuadPart);

        /* 动态申请空间 */
        tmpView = MALLOC(bufferSize);

        if(tmpView)
        {
            /* 对OVERLAPPED结构清0 */
            ZeroMemory(&ol, sizeof(ol));

            /* 设置偏移量 */
            ol.Offset = alignedBegin.LowPart;
            ol.OffsetHigh = alignedBegin.HighPart;

            /* 在指定位置读取数据 */
            if(ReadFile(media->hFile, tmpView, bufferSize, &readBytes, &ol))//拷贝数据成功
            {
                FREE(media->pView);

                media->pView = tmpView;
                media->accessSize = len;

                retVal = SUCCESS;
            }
            else//拷贝数据失败
                FREE(tmpView);
        }
    }

    if(retVal == SUCCESS || len <= media->accessSize)
    {
        access->begin = (unsigned char *)media->pView + (media->currPos.QuadPart & (media->viewSize - 1));
        access->len = len;

        retVal = SUCCESS;
    }

    return retVal;
}

int GetMediaAccess(T media, media_access *access, uint32_t len)
{
    int retVal = FAILED;

    /* 参数检查，并进行越界检查 */
    if(media && access && len && media->currPos.QuadPart + len < media->size.QuadPart)
    {
        if(media->allocGran)//如果是映射文件
            retVal = MapMediaAccess(media, access, len);
        else//如果是打开介质
            retVal = RawMediaAccess(media, access, len);
    }
    
    return retVal;
}

static int DumpMapMedia(T media, FILE *fp, int64_t size)
{
    int retVal = FAILED;

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

    return retVal;
}

static int DumpRawMedia(T media, FILE *fp, int64_t size)
{
    int retVal = FAILED;

    if(size <= (int64_t)media->accessSize)
    {
        /* 直接把缓冲区中内容写入fp */
        if(fwrite(media->pView, size, 1, fp) == 1)
            retVal = SUCCESS;
    }
    else
    { 
        void *buffer;                   //缓冲区指针，动态申请，大小为1个扇区大小
        LARGE_INTEGER offset = {media->currPos.QuadPart};//代替ol的偏移量进行计算
        LARGE_INTEGER alignedBegin;     //对齐开始偏移量
        LARGE_INTEGER alignedEnd;       //对齐结束偏移量
        OVERLAPPED ol;                  //读取偏移量，用于ReadFile
        DWORD readBytes;                //读取到输出缓冲区的数据大小
        uint32_t sectors;               //需要读取的扇区数

        assert((media->currPos.QuadPart & ~(media->viewSize - 1)) 
            == (media->currPos.QuadPart / media->viewSize) * media->viewSize);

        /* 对齐开始偏移量 */
        alignedBegin.QuadPart = media->currPos.QuadPart & ~(media->viewSize - 1);

        /* 赋值未对齐的结尾位置到alignedEnd */
        alignedEnd.QuadPart = media->currPos.QuadPart + size;

        /* 对齐结束偏移量 */
        if(alignedEnd.QuadPart % media->viewSize)//未对齐
            alignedEnd.QuadPart = (alignedEnd.QuadPart & ~(media->viewSize - 1)) + media->viewSize;

        assert(alignedEnd.QuadPart < media->size.QuadPart);
        assert(alignedEnd.QuadPart - alignedBegin.QuadPart >= 0);
        assert(alignedEnd.QuadPart - alignedBegin.QuadPart <= UINT_MAX);
        assert(!((alignedEnd.QuadPart - alignedBegin.QuadPart) % media->viewSize));

        sectors = (alignedEnd.QuadPart - alignedBegin.QuadPart) / media->viewSize;

        /* 动态申请空间 */
        buffer = MALLOC(media->viewSize);

        if(buffer && sectors)
        {
            DWORD result;

            /* 对OVERLAPPED结构清0 */
            ZeroMemory(&ol, sizeof(ol));

            /* 设置偏移量 */
            ol.Offset = alignedBegin.LowPart;
            ol.OffsetHigh = alignedBegin.HighPart;

            /* 读取第一个扇区 */
            result = ReadFile(media->hFile, buffer, media->viewSize, &readBytes, &ol);

            if(result)
            {
                uint32_t alignCost = (uint32_t)(media->currPos.QuadPart & (media->viewSize - 1));//对齐消耗偏移量

                /* 写入第一个扇区的指定部分 */
                result = fwrite((unsigned char *)buffer + alignCost, media->viewSize - alignCost, 1, fp);
            }

            if(result)
            {
                result = 0;

                if(--sectors)//扇区数-1 > 0
                {
                    int i;

                    alignedBegin.QuadPart += media->viewSize;

                    /* 设置偏移量 */
                    ol.Offset = alignedBegin.LowPart;
                    ol.OffsetHigh = alignedBegin.HighPart;

                    /* 循环写入中间部分扇区，留最后1扇区后面处理 */
                    for(i = 0; i < sectors - 1; ++i)
                    {
                        if(ReadFile(media->hFile, buffer, media->viewSize, &readBytes, &ol))
                            if(fwrite(buffer, media->viewSize, 1, fp))
                            {
                                alignedBegin.QuadPart += media->viewSize;

                                /* 设置偏移量 */
                                ol.Offset = alignedBegin.LowPart;
                                ol.OffsetHigh = alignedBegin.HighPart;

                                continue;
                            }

                        break;
                    }

                    if(i == sectors - 1)
                        result = 1;//拷贝成功
                }
            }

            if(result)
            {
                /* 读取最后一个扇区 */
                if(ReadFile(media->hFile, buffer, media->viewSize, &readBytes, &ol))
                {
                    uint32_t writeSize = (uint32_t)(media->viewSize - 
                        (alignedEnd.QuadPart - (media->currPos.QuadPart + size)));

                    if(writeSize && fwrite(buffer, writeSize, 1, fp))
                        retVal = SUCCESS;
                }
            }
        }

        FREE(buffer);
    }

    return retVal;
}

int DumpMedia(T media, FILE *fp, int64_t size)
{
    int retVal = FAILED;

    /* 参数检查，并进行越界检查 */
    if(media && fp && size > 0 && media->currPos.QuadPart + size < media->size.QuadPart)
    {
        LARGE_INTEGER currPos = media->currPos;

#ifdef _DEBUG
        media_t media_bak;    
        assert(NEW(media_bak));
        memcpy(media_bak, media, sizeof(*media));
#endif

        if(media->allocGran)//如果是文件映射
            retVal = DumpMapMedia(media, fp, size);
        else//如果是直接打开
            retVal = DumpRawMedia(media, fp, size);

        if(SeekMedia(media, currPos.QuadPart, MEDIA_SET) != SUCCESS)
            retVal = FAILED;

#ifdef _DEBUG
        assert(!memcmp(media_bak, media, sizeof(*media)));
        FREE(media_bak);
#endif
    }

    return retVal;
}
