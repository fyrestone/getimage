/*!
\file MapFile.c
\author LiuBao
\version 1.1
\date 2011/2/10
\brief 介质操作函数实现

1.0 - 2010/12/28: 初始版本
1.1 - 2011/2/10: 
    + 修正多处BUG
    + 增加设备操作支持，
    + 功能点完成
*/

#define WINVER 0x0500               ///< 为了使用GetFileSizeEx

#include <Windows.h>                /* 使用文件映射 */
#include <io.h>                     /* _access，检测文件存在 */
#include <assert.h>
#include <limits.h>                 /* 使用INT_MAX */
#include "AprRing\apr_ring.h"       /* APR环实现 */
#include "ProjDef.h"
#include "MapFile.h"
#include "SafeMemory.h"

struct elem_t                               ///< APR环元素类型定义
{
    APR_RING_ENTRY(elem_t) link;            ///< 链接域
    void *pBuffer;                          ///< 数据域，动态申请的缓冲区地址
    media_access *access;                   ///< 数据域，access结构体地址
};

APR_RING_HEAD(elem_head_t, elem_t);         ///< APR环头结点定义

enum MediaType                              ///< 介质类型枚举
{
    Map,                                    ///< 文件映射
    Raw                                     ///< 直接IO
};

#define T media_t                           ///< 为抽象数据类型T定义实际名

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

    enum MediaType type;                    ///< 介质类型
};

/* 输出重定向器定义 */
typedef int (*Redirector)(const void *pBuffer, uint32_t size, void *data);

/* 局部函数前置声明 */
static int FileOut(const void *pBuffer, uint32_t size, void *data);
static int MemOut(const void *pBuffer, uint32_t size, void *data);
static void ClearAccessRing(struct elem_head_t *head);
static HANDLE MapMedia(const _TCHAR *path, PLARGE_INTEGER lpFileSize);
static HANDLE RawMedia(const _TCHAR *path, PLARGE_INTEGER lpFileSize, uint32_t *lpBlockSize);
static int SplitMapView(T media, int64_t offset);
static int FullMapView(T media, uint32_t offset);
static int SeekMapMedia(T media, int64_t offset, int base);
static int SeekRawMedia(T media, int64_t offset, int base);
static T OpenMapMedia(T media, const _TCHAR *path, uint32_t viewSize);
static T OpenRawMedia(T media, const _TCHAR *path);
static int MapMediaAccess(T media, media_access *access, uint32_t len);
static int RawMediaAccess(T media, media_access *access, uint32_t len);
static int DumpMapMedia(T media, int64_t size, Redirector processor, void *data);
static int DumpRawMedia(T media, int64_t size, Redirector processor, void *data);

/*!
把缓冲区数据从文件指针输出
\param pBuffer 缓冲区指针
\param size 缓冲区大小
\param data 文件指针
\return 输出成功返回SUCCESS；否则返回FAILED
*/
static int FileOut(const void *pBuffer, uint32_t size, void *data)
{
    return fwrite(pBuffer, size, 1, (FILE *)data) == 1 ? SUCCESS : FAILED;
}

/*!
把缓冲区数据输出到另一块内存
\param pBuffer 缓冲区指针
\param size 缓冲区大小
\param data 目标内存地址指针（二级指针）
\return 输出成功返回SUCCESS；否则返回FAILED
*/
static int MemOut(const void *pBuffer, uint32_t size, void *data)
{
    int retVal = FAILED;

    if(memcpy(*(unsigned char **)data, pBuffer, size))
    {
        *(unsigned char **)data += size;

        retVal = SUCCESS;
    }

    return retVal;
}

/*!
由访问链表头指针，清空访问链表
\param head 访问链表头
*/
static void ClearAccessRing(struct elem_head_t *head)
{
    struct elem_t *elem = NULL;

    while(!APR_RING_EMPTY(head, elem_t, link))
    {
        if((elem = APR_RING_FIRST(head)))
        {
            APR_RING_REMOVE(elem, link);

            elem->access->begin = 0;
            elem->access->len = 0;

            FREE(elem);
            elem = NULL;
        }
    }
}

/*!
获得映射介质大小（通常是可访问文件）
\param path 映射介质路径
\param lpFileSize 映射介质大小指针
\return 获得映射介质大小成功，返回映射句柄；否则返回INVALID_HANDLE_VALUE
*/
static HANDLE MapMedia(const _TCHAR *path, PLARGE_INTEGER lpFileSize)
{
    HANDLE hFileMapping = INVALID_HANDLE_VALUE;

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

/*!
返回设备等介质大小（通常是设备）
\param path 设备路径
\param lpFileSize 设备大小指针
\param lpSectorSize 设备扇区大小
\return 获得设备介质大小成功，返回设备句柄；否则返回INVALID_HANDLE_VALUE
*/
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

/*!
分块映射时，调整media_t的当前位置到offset；并根据情况自动切换视图
\param media 分块映射介质
\param offset 要修改为的偏移量
\return 调整成功返回SUCCESS；否则返回FAILED
*/
static int SplitMapView(T media, int64_t offset)
{
#define ALIGN_COST (offset & (media->map.allocGran - 1)) //对齐消耗的大小

    int retVal = FAILED;

    LARGE_INTEGER alignedOffset;

    assert((offset & ~(media->map.allocGran - 1)) == (offset / media->map.allocGran) * media->map.allocGran);

    /* 偏移量与内存粒度对齐 */
    alignedOffset.QuadPart = offset & ~(media->map.allocGran - 1);

    if(media->map.pView && alignedOffset.QuadPart == media->map.viewPos.QuadPart)
    {
        media->map.currPos.QuadPart = offset;
        media->map.accessSize = media->map.actualViewSize - ALIGN_COST;

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
        if(alignedOffset.QuadPart + media->map.viewSize > media->map.size.QuadPart)
            actualViewSize = (uint32_t)(media->map.size.QuadPart - alignedOffset.QuadPart);
        else
            actualViewSize = media->map.viewSize;

        pNewView = MapViewOfFile(
            media->map.hFile, FILE_MAP_READ, alignedOffset.HighPart, alignedOffset.LowPart, actualViewSize);

        if(pNewView)
        {
            /* 若media存在旧映射视图,并且销毁成功,或者media不存在旧映射视图 */
            if((media->map.pView && UnmapViewOfFile(media->map.pView)) || !media->map.pView)
            {
                media->map.pView = pNewView;
                media->map.viewPos = alignedOffset;
                media->map.actualViewSize = actualViewSize;

                /*
                可访问视图大小应当等于实际的视图大小actualViewSize减去
                对齐offset时消耗的大小，即offset-alignedOffset，
                这里优化一下使用位运算提高速度。
                */
                media->map.currPos.QuadPart = offset;
                media->map.accessSize = actualViewSize - ALIGN_COST;

                retVal = SUCCESS;
            }
        }
    }

    return retVal;

#undef ALIGN_COST
}

/*!
完全映射时，调整media_t的当前位置到offset
\param media 完全映射介质
\param offset 要修改为的偏移量
\return 调整成功返回SUCCESS；否则返回FAILED
*/
static int FullMapView(T media, uint32_t offset)
{
    int retVal = FAILED;

    if(media->map.pView)//如果已经映射成功
    {
        /*
        offset已经是绝对偏移量，这里可访问视图大小accessSize
        直接是实际视图大小（同样也是介质大小）减去offset即可
        */
        media->map.currPos.QuadPart = offset;
        media->map.accessSize = media->map.actualViewSize - offset;

        retVal = SUCCESS;
    }
    else//如果还未映射，则进行完全映射
    {
        PVOID pNewView = MapViewOfFile(
            media->map.hFile, FILE_MAP_READ, 0, 0, 0);

        if(pNewView)
        {
            media->map.pView = pNewView;

            /*
            首次（也是唯一一次）进行完全映射后，
            可访问视图大小和实际视图大小应当与介质
            大小相同
            */
            media->map.accessSize = media->map.size.LowPart;
            media->map.actualViewSize = media->map.size.LowPart;

            retVal = SUCCESS;
        }
    }    

    return retVal;
}

/*!
以文件映射方式打开介质
\param media 未初始化的介质
\param path 介质路径
\param viewSize 映射使用的视图大小，函数将自动将其对齐到内存分配粒度
\return 打开介质成功，返回初始化完成的media_t；否则返回NULL
*/
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
                    media->type = Map;
                    APR_RING_INIT(&media->map.accessRing, elem_t, link);

                    media->map.name = path;
                    media->map.hFile = hFileMapping;
                    media->map.allocGran = sysInfo.dwAllocationGranularity;
                    media->map.size = fileSize;
                    media->map.viewSize = viewSize;
                    media->map.accessSize = 0;
                    media->map.actualViewSize = 0;
                    media->map.viewPos.QuadPart = 0;
                    media->map.currPos.QuadPart = 0;
                    media->map.pView = NULL;

                    if(RewindMedia(media) == SUCCESS)
                        retVal = media;
                }
            }
        }
    }

    return retVal;
}

/*!
以直接IO方式打开介质
\param media 未初始化的介质
\param path 介质路径
\return 打开介质成功，返回初始化完成的media_t；否则返回NULL
*/
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
            media->type = Raw;

            media->raw.name = path;
            media->raw.hFile = hFile;
            media->raw.size = fileSize;
            media->raw.currPos.QuadPart = 0;
            media->raw.sectorSize = sectorSize;
            media->raw.accessSize = 0;
            media->raw.pBuffer = 0;

            retVal = media;
        }
    }

    return retVal;
}

int RewindMedia(T media)
{
    return SeekMedia(media, 0, MEDIA_SET);
}

/*!
文件映射时，根据base跳转介质当前位置到offset
\param media 初始化完成的介质
\param offset 偏移量
\param base 可能取值为MEDIA_CUR（相对于当前位置）和MEDIA_SET（相对于起始位置）
\return 跳转成功，返回SUCCESS；否则返回FAILED
*/
static int SeekMapMedia(T media, int64_t offset, int base)
{
    int retVal = FAILED;

    LARGE_INTEGER mendedOffset;

    mendedOffset.QuadPart = offset;

    /* 修正偏移量至绝对偏移量 */
    if(base == MEDIA_CUR)
        mendedOffset.QuadPart += media->map.currPos.QuadPart;

    /* 如果修正后偏移量mendedOffset没有超出文件范围 */
    if(mendedOffset.QuadPart >= 0 && mendedOffset.QuadPart < media->map.size.QuadPart)
    {
        if(media->map.viewSize)//如果分块映射
            retVal = SplitMapView(media, mendedOffset.QuadPart);
        else//如果完全映射
            retVal = FullMapView(media, mendedOffset.LowPart);
    }

    return retVal;
}

/*!
直接IO时，根据base跳转介质当前位置到offset
\param media 初始化完成的介质
\param offset 偏移量
\param base 可能取值为MEDIA_CUR（相对于当前位置）和MEDIA_SET（相对于起始位置）
\return 跳转成功，返回SUCCESS；否则返回FAILED
*/
static int SeekRawMedia(T media, int64_t offset, int base)
{
    int retVal = FAILED;

    /* 修正偏移量至绝对偏移量 */
    if(base == MEDIA_CUR)
        offset += media->raw.currPos.QuadPart;

    /* 如果修正后偏移量offset没有超出文件范围 */
    if(offset >= 0 && offset < media->raw.size.QuadPart)
    {
        if(media->raw.currPos.QuadPart != offset)//如果Seek后位置改变
        {
            media->raw.accessSize = 0;              //可访问大小置0
            media->raw.currPos.QuadPart = offset;   //修改当前位置
        }

        retVal = SUCCESS;
    }

    return retVal;
}

int SeekMedia(T media, int64_t offset, int base)
{
    int retVal = FAILED;

    if(media && (base == MEDIA_SET || base == MEDIA_CUR))
    {
        if(media->type == Map)//如果是映射文件
        {
            ClearAccessRing(&media->map.accessRing);
            retVal = SeekMapMedia(media, offset, base);
        }
        else if(media->type == Raw)//如果是打开介质
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
        if(mediaCopy->type == Map)//如果是映射文件
        {
            UnmapViewOfFile(mediaCopy->map.pView);          /* 卸载视图 */
            CloseHandle(mediaCopy->map.hFile);              /* 关闭映射句柄 */
            ClearAccessRing(&mediaCopy->map.accessRing);    /* 清空访问链表 */
        }
        else if(mediaCopy->type == Raw)//如果是打开介质
        {
            FREE(mediaCopy->raw.pBuffer);                   /* 释放缓冲区 */
            CloseHandle(mediaCopy->raw.hFile);              /* 关闭介质句柄 */
        }

        FREE(mediaCopy);                                    /* 释放media_t结构 */
        *media = NULL;                                      /* 参数指向的media_t置NULL */
    }
}

/*!
从当前位置，获得指定有效数据长度的映射介质的访问指针，填充media_access结构体
\param media 初始化完成的介质（并且是以文件映射方式打开）
\param access 为初始化访问结构体
\param len 有效数据长度
\return 操作成功返回SUCCESS；否则返回FAILED
*/
static int MapMediaAccess(T media, media_access *access, uint32_t len)
{
    int retVal = FAILED;

    /* 越界检查 */
    if(media->map.currPos.QuadPart + len <= media->map.size.QuadPart)
    {
        if(len <= media->map.accessSize)
        {
            if(media->map.viewSize)//如果分块映射
                access->begin = (const unsigned char *)
                media->map.pView + media->map.viewSize - media->map.accessSize;
            else//如果完全映射
                access->begin = (const unsigned char *)
                media->map.pView + media->map.currPos.LowPart;

            access->len = len;

            retVal = SUCCESS;
        }
        else
        {
            void *buffer = MALLOC(len);

            if(buffer)
            {
                void *writePtr = buffer;//备份buffer到writePtr
                retVal = DumpMapMedia(media, len, MemOut, &writePtr);

                assert(((char *)writePtr - (char *)buffer) == len);

                if(retVal == SUCCESS)
                {
                    struct elem_t *el = (struct elem_t *)MALLOC(sizeof(struct elem_t));
                    
                    if(el)
                    {
                        el->access = access;
                        el->pBuffer = buffer;

                        APR_RING_ELEM_INIT(el, link);
                        APR_RING_INSERT_TAIL(&media->map.accessRing, el, elem_t, link);

                        access->begin = (const unsigned char *)buffer;
                        access->len = len;
                    }
                }
                else
                    FREE(buffer);
            }
        }
    }

    return retVal;
}

/*!
从当前位置，获得指定有效数据长度的直接IO介质的访问指针，填充media_access结构体
\param media 初始化完成的介质（并且是以直接IO方式打开）
\param access 为初始化访问结构体
\param len 有效数据长度
\return 操作成功返回SUCCESS；否则返回FAILED
*/
static int RawMediaAccess(T media, media_access *access, uint32_t len)
{
    int retVal = FAILED;

    /* 越界检查 */
    if(media->raw.currPos.QuadPart + len <= media->raw.size.QuadPart)
    {
        /* 切换pBuffer，释放旧pBuffer，申请新pBuffer，并把填写新pBuffer区域 */
        if(len > media->raw.accessSize)//若accessSize为0 或 len > accessSize
        {
            void *tmpView;                  //申请缓冲区空间
            DWORD readBytes;                //读取到输出缓冲区的数据大小
            OVERLAPPED ol;                  //读取偏移量
            LARGE_INTEGER alignedBegin;     //对齐开始偏移量
            LARGE_INTEGER alignedEnd;       //对齐结束偏移量
            uint32_t bufferSize;            //缓冲数据大小

            assert((media->raw.currPos.QuadPart & ~(media->raw.sectorSize - 1)) 
                == (media->raw.currPos.QuadPart / media->raw.sectorSize) * media->raw.sectorSize);

            /* 对齐开始偏移量 */
            alignedBegin.QuadPart = media->raw.currPos.QuadPart & ~(media->raw.sectorSize - 1);

            /* 赋值未对齐的结尾位置到alignedEnd */
            alignedEnd.QuadPart = media->raw.currPos.QuadPart + len;

            /* 对齐结束偏移量 */
            if(alignedEnd.QuadPart % media->raw.sectorSize)//未对齐
                alignedEnd.QuadPart = (alignedEnd.QuadPart & ~(media->raw.sectorSize - 1)) + media->raw.sectorSize;

            assert(alignedEnd.QuadPart < media->raw.size.QuadPart);
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
                if(ReadFile(media->raw.hFile, tmpView, bufferSize, &readBytes, &ol))//拷贝数据成功
                {
                    FREE(media->raw.pBuffer);

                    media->raw.pBuffer = tmpView;
                    media->raw.accessSize = len;

                    retVal = SUCCESS;
                }
                else//拷贝数据失败
                    FREE(tmpView);
            }
        }

        if(retVal == SUCCESS || len <= media->raw.accessSize)
        {
            access->begin = (unsigned char *)media->raw.pBuffer + (media->raw.currPos.QuadPart & (media->raw.sectorSize - 1));
            access->len = len;

            retVal = SUCCESS;
        }
    }

    return retVal;
}

int GetMediaAccess(T media, media_access *access, uint32_t len)
{
    int retVal = FAILED;

    if(media && access && len)
    {
        if(media->type == Map)//如果是映射文件
            retVal = MapMediaAccess(media, access, len);
        else if(media->type == Raw)//如果是打开介质
            retVal = RawMediaAccess(media, access, len);
    }

    return retVal;
}

/*!
文件映射时，从当前位置开始，抽取指定长度的数据，用Redirector转发该段数据
\param media 初始化完成的介质（并且是以文件映射方式打开）
\param size 抽取数据长度
\param processor 数据重定向器
\param data 附加参数，用于传递参数给数据重定向器
\return 抽取成功，返回SUCCESS；否则返回FAILED
*/
static int DumpMapMedia(T media, int64_t size, Redirector processor, void *data)
{
    int retVal = FAILED;

    /* 越界检查 */
    if(media->map.currPos.QuadPart + size <= media->map.size.QuadPart)
    {
        int hasError = 0;//错误标记
        LARGE_INTEGER currPos = media->map.currPos;//保存当前位置，以便结束时恢复       

        while(size > media->map.accessSize)
        {
            const unsigned char *writePtr = (const unsigned char *)
                media->map.pView + media->map.viewSize - media->map.accessSize;

            if(processor(writePtr, media->map.accessSize, data) == SUCCESS)
            {
                uint32_t writeLen = media->map.accessSize;//保存写入的长度

                /* 向后跳转，会修改media->map.accessSize */
                if(SeekMapMedia(media, media->map.accessSize, MEDIA_CUR) == SUCCESS)
                {
                    size -= writeLen;

                    continue;
                }
            }

            hasError = 1;//错误标记置位

            break;  
        }

        if(!hasError)
        {
            uint32_t offset = (uint32_t)(media->map.currPos.QuadPart - media->map.viewPos.QuadPart);

            if(size && processor((const unsigned char *)media->map.pView + offset, (size_t)size, data) == SUCCESS)
                retVal = SUCCESS;
        }

        if(SeekMapMedia(media, currPos.QuadPart, MEDIA_SET) != SUCCESS)
            retVal = FAILED;
    }

    return retVal;
}

/*!
直接IO时，从当前位置开始，抽取指定长度的数据，用Redirector转发该段数据
\param media 初始化完成的介质（并且是以直接IO方式打开）
\param size 抽取数据长度
\param processor 数据重定向器
\param data 附加参数，用于传递参数给数据重定向器
\return 抽取成功，返回SUCCESS；否则返回FAILED
*/
static int DumpRawMedia(T media, int64_t size, Redirector processor, void *data)
{
    int retVal = FAILED;

    if(media->raw.currPos.QuadPart + size <= media->raw.size.QuadPart)
    {
        if(size <= (int64_t)media->raw.accessSize)
        {
            /* 直接处理缓冲区中内容 */
            retVal = processor(media->raw.pBuffer, (uint32_t)size, data);
        }
        else
        { 
            void *buffer;                   //缓冲区指针，动态申请，大小为1个扇区大小
            //LARGE_INTEGER offset = media->raw.currPos;//代替ol的偏移量进行计算
            LARGE_INTEGER alignedBegin;     //对齐开始偏移量
            LARGE_INTEGER alignedEnd;       //对齐结束偏移量
            OVERLAPPED ol;                  //读取偏移量，用于ReadFile
            DWORD readBytes;                //读取到输出缓冲区的数据大小
            uint32_t sectors;               //需要读取的扇区数

            assert((media->raw.currPos.QuadPart & ~(media->raw.sectorSize - 1)) 
                == (media->raw.currPos.QuadPart / media->raw.sectorSize) * media->raw.sectorSize);

            /* 对齐开始偏移量 */
            alignedBegin.QuadPart = media->raw.currPos.QuadPart & ~(media->raw.sectorSize - 1);

            /* 赋值未对齐的结尾位置到alignedEnd */
            alignedEnd.QuadPart = media->raw.currPos.QuadPart + size;

            /* 对齐结束偏移量 */
            if(alignedEnd.QuadPart % media->raw.sectorSize)//未对齐
                alignedEnd.QuadPart = (alignedEnd.QuadPart & ~(media->raw.sectorSize - 1)) + media->raw.sectorSize;

            assert(alignedEnd.QuadPart < media->raw.size.QuadPart);
            assert(alignedEnd.QuadPart - alignedBegin.QuadPart >= 0);
            assert(alignedEnd.QuadPart - alignedBegin.QuadPart <= UINT_MAX);
            assert(!((alignedEnd.QuadPart - alignedBegin.QuadPart) % media->raw.sectorSize));

            sectors = (uint32_t)((alignedEnd.QuadPart - alignedBegin.QuadPart) / media->raw.sectorSize);

            /* 动态申请空间 */
            buffer = MALLOC(media->raw.sectorSize);

            if(buffer && sectors)
            {
                DWORD result;

                /* 对OVERLAPPED结构清0 */
                ZeroMemory(&ol, sizeof(ol));

                /* 设置偏移量 */
                ol.Offset = alignedBegin.LowPart;
                ol.OffsetHigh = alignedBegin.HighPart;

                /* 读取第一个扇区 */
                result = ReadFile(media->raw.hFile, buffer, media->raw.sectorSize, &readBytes, &ol);

                if(result)
                {
                    uint32_t alignCost = (uint32_t)
                        (media->raw.currPos.QuadPart & (media->raw.sectorSize - 1));//对齐消耗偏移量

                    /* 写入第一个扇区的指定部分 */
                    int processVal = processor(
                        (const unsigned char *)buffer + alignCost, //数据开始指针
                        media->raw.sectorSize - alignCost,         //数据块大小
                        data);                                     //附加参数指针

                    result = processVal == SUCCESS ? 1 : 0;
                }

                if(result)
                {
                    result = 0;

                    if(--sectors)//扇区数-1 > 0
                    {
                        uint32_t i;

                        alignedBegin.QuadPart += media->raw.sectorSize;

                        /* 设置偏移量 */
                        ol.Offset = alignedBegin.LowPart;
                        ol.OffsetHigh = alignedBegin.HighPart;

                        /* 循环写入中间部分扇区，留最后1扇区后面处理 */
                        for(i = 0; i < sectors - 1; ++i)
                        {
                            if(ReadFile(media->raw.hFile, buffer, media->raw.sectorSize, &readBytes, &ol) &&
                                processor(buffer, media->raw.sectorSize, data) == SUCCESS)
                            {
                                alignedBegin.QuadPart += media->raw.sectorSize;

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
                    if(ReadFile(media->raw.hFile, buffer, media->raw.sectorSize, &readBytes, &ol))
                    {
                        uint32_t writeSize = (uint32_t)(media->raw.sectorSize - 
                            (alignedEnd.QuadPart - (media->raw.currPos.QuadPart + size)));

                        if(writeSize && processor(buffer, writeSize, data) == SUCCESS)
                            retVal = SUCCESS;
                    }
                }
            }

            FREE(buffer);
        }
    }

    return retVal;
}

int DumpMedia(T media, FILE *fp, int64_t size)
{
    int retVal = FAILED;

    if(media && fp && size > 0)
    {
#ifdef _DEBUG
        media_t media_bak;    
        assert(NEW(media_bak));
        memcpy(media_bak, media, sizeof(*media));
#endif

        if(media->type == Map)//如果是文件映射
            retVal = DumpMapMedia(media, size, FileOut, fp);
        else if(media->type == Raw)//如果是直接打开
            retVal = DumpRawMedia(media, size, FileOut, fp);

#ifdef _DEBUG
        /* media与media_bak不一定完全一样，比如缓冲区地址 */
        assert(!memcmp(media_bak, media, sizeof(*media)));
        FREE(media_bak);
#endif
    }

    return retVal;
}
