/*!
\file Process.c
\author LiuBao
\version 1.0
\date 2011/1/23
\brief 处理函数实现
*/
#include <Windows.h>     /* 使用MAX_PATH宏 */
#include <io.h>          /* _access，检测文件存在 */
#include <stdio.h>       /* 使用FILE */
#include <assert.h>
#include "ProjDef.h"
#include "ColorPrint.h"
#include "Process.h"
#include "IMGProcess.h"
#include "ISOProcess.h"

static _TCHAR PATH[MAX_PATH];   /* GetOutPath输出缓冲区，每次调用都会覆盖 */

/*!
从img起始位置提取出img写入到path路径
\param imageEntry 位置在img起始的media_t
\param path img写入路径
\return 成功返回SUCCESS；否则返回FAILED
*/
static int WriteIMGToFile(media_t imageEntry, const _TCHAR *path)
{
    int retVal = FAILED;
    
    if(imageEntry && path)
    {
        media_access access;

        if(GetMediaAccess(imageEntry, &access, sizeof(BPB)) == SUCCESS)
        {
            const BPB *pcBPB = (const BPB *)access.begin;
            size_t totalSize = 0;//IMG大小

            /* 获得整个IMG大小 */
            {
                size_t totalSec = 0;//总扇区数

                if(LD_UINT16(pcBPB->Common.BPB_TotSec16))//如果是FAT12/16
                    totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
                else//如果是FAT32
                    totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

                totalSize = totalSec * LD_UINT16(pcBPB->Common.BPB_BytsPerSec);
            }

            /* 把IMG写入文件 */
            {
                FILE *outfp = _tfopen(path, _T("wb"));

                if(DumpMedia(imageEntry, outfp, totalSize) == SUCCESS)
                    retVal = SUCCESS;

                fclose(outfp);
            }
        }
    }

    return retVal;
}

/*!
从ISO文件头部跳转到启动IMG头部，内部自动处理Acronis启动光盘
\param media ISO文件的media_t
\return 成功返回SUCCESS；否则返回FAILED
*/
static int JumpToISOBootEntry(media_t media)
{
    int retVal = FAILED;

    if(media)
    {
        /*连续跳转指针*/
        if(JumpToISOPrimVolDesc(media) == SUCCESS &&
            JumpToISOBootRecordVolDesc(media) == SUCCESS &&
            JumpToISOValidationEntry(media) == SUCCESS &&
            JumpToISOInitialEntry(media) == SUCCESS &&
            JumpToISOBootableImage(media) == SUCCESS)
        {

            /*尝试从ISO的BootImage入口跳转到IMG入口*/
            if(CheckIMGIdentifier(media) != SUCCESS)
            {
                media_access access;
                if(GetMediaAccess(media, &access, 512) == SUCCESS)
                {
                    /* Acronis光盘特殊跳转 */
                    uint32_t offsetValue = LD_UINT8(access.begin + 244);
                    uint32_t relativeOffset = offsetValue * SECTOR_SIZE/*offsetUnit*/;
                    
                    (void)SeekMedia(media, relativeOffset, MEDIA_CUR);
                }
            }
        }

        /* 检查入口是否为FAT格式 */
        {
            media_access access;
            if(GetMediaAccess(media, &access, 512) == SUCCESS)
            {
                if(CheckIMGIdentifier(media) == SUCCESS &&
                    CheckIMGFileSystem(media) == SUCCESS)
                    retVal = SUCCESS;//找到IMG入口
            }
        }
    }

    return retVal;
}

const _TCHAR *GetOutPath(const _TCHAR *path, const _TCHAR *extention)
{
    const _TCHAR *retVal = NULL;

    if(!_taccess(path, 0/* 检测存在 */))//如果文件存在
    {
        const _TCHAR *endOfSelfPath = path + _tcslen(path);//初始化指向路径字串结尾符

        /* 找到扩展名左边的“.” */
        while(*endOfSelfPath != '.' && endOfSelfPath != path)
            --endOfSelfPath;

        if(endOfSelfPath != path)
        {
            _TCHAR *target = PATH;

            /* 拷贝ISO路径的扩展名前部分 */
            while(path != endOfSelfPath)
                *target++ = *path++;

            /* 写入扩展名 */
            while((*target++  = *extention++));

            retVal = PATH;//获取输出路径
        }
    }

    return retVal;
}

MEDIA_TYPE GetInputType(media_t media)
{
    MEDIA_TYPE retVal = UNKNOWN;

    if(media)
    {
        if(CheckIMGIdentifier(media) == SUCCESS)
            retVal = IMG;//是IMG格式
        else if(JumpToISOPrimVolDesc(media) == SUCCESS)
        {
            (void)RewindMedia(media);
            retVal = ISO;//是ISO格式
        }
    }
    
    return retVal;
}

int DumpIMGFromISO(media_t media, const _TCHAR *imgPath)
{
    static const _TCHAR * const leftMargin = _T("    ");//左间距

    int retVal = FAILED;

    if(media && imgPath)
    {
        ColorPrintf(WHITE, _T("尝试从ISO文件中提取IMG:\n\n"));
        ColorPrintf(WHITE, _T("%s正在寻找IMG入口\t\t\t"), leftMargin);

        do
        {
            /* 跳转到IMG头部 */
            if(JumpToISOBootEntry(media) == SUCCESS)
            {
                ColorPrintf(LIME, _T("成功\n"));
                ColorPrintf(WHITE, _T("%s正在写入IMG文件\t\t\t"), leftMargin);
            }
            else
            {
                ColorPrintf(RED, _T("失败\n"));
                break;
            }

            /* 把IMG写入文件 */
            if(WriteIMGToFile(media, imgPath) == SUCCESS)
            {
                ColorPrintf(LIME, _T("成功\n"));
                ColorPrintf(WHITE, _T("\n处理完毕\n"));
            }
            else
            {
                ColorPrintf(RED, _T("失败\n"));
                break;
            }

            retVal = SUCCESS;//写入成功！
        }while(0);
    }

    return retVal;
}

int DisplayISOInfo(media_t media)
{
    static const _TCHAR * const leftMargin = _T("    ");//左间距

    int retVal = FAILED;

    if(media)
    {
        media_access access;

        ColorPrintf(WHITE, _T("检测到ISO文件信息如下：\n\n"));

        /* 输出PrimVolDesc中信息 */
        if(JumpToISOPrimVolDesc(media) == SUCCESS)
        {
            if(GetMediaAccess(media, &access, sizeof(PrimVolDesc)) == SUCCESS)
            {
                const PrimVolDesc *pcPVD = (const PrimVolDesc *)access.begin;
                const ISO9660TimeStr *createDate = (const ISO9660TimeStr *)(pcPVD->VolCreationDate);
                const ISO9660TimeStr *modifyDate = (const ISO9660TimeStr *)(pcPVD->VolModifDate);

                uint32_t volBlocks = LD_UINT32(pcPVD->VolSpaceSz);//ISO卷逻辑块数
                uint16_t bytsPerBlocks = LD_UINT16(pcPVD->LogicalBlockSz);//逻辑块所占字节数

                ColorPrintf(WHITE, _T("%s光盘标签：\t\t\t\t"), leftMargin);
                ColorPrintfA(LIME, "%.32s\n", pcPVD->VolID);
                ColorPrintf(YELLOW, _T("%s卷逻辑块数：\t\t\t"), leftMargin);
                ColorPrintf(AQUA, _T("%u\n"), volBlocks);
                ColorPrintf(YELLOW, _T("%s每逻辑块字节数：\t\t\t"), leftMargin);
                ColorPrintf(AQUA, _T("%u\n"), bytsPerBlocks);
                ColorPrintf(YELLOW, _T("%s光盘容量：\t\t\t\t"), leftMargin);
                ColorPrintf(AQUA, _T("%u\n"), volBlocks * bytsPerBlocks);

                /* 输出创建日期和修改日期 */
                if(createDate && modifyDate)
                {
                    ColorPrintf(WHITE, _T("%s创建日期：\t\t\t\t"), leftMargin);
                    ColorPrintfA(LIME, "%.4s/%.2s/%.2s %.2s:%.2s:%.2s\n",
                        createDate->Year, createDate->Month, createDate->Day,
                        createDate->Hour, createDate->Minute, createDate->Second);

                    ColorPrintf(WHITE, _T("%s修改日期：\t\t\t\t"), leftMargin);
                    ColorPrintfA(LIME, "%.4s/%.2s/%.2s %.2s:%.2s:%.2s\n",
                        createDate->Year, createDate->Month, createDate->Day,
                        createDate->Hour, createDate->Minute, createDate->Second);
                }
            }
        }

        /* 输出BootRecordVolDesc中信息 */
        if(JumpToISOBootRecordVolDesc(media) == SUCCESS)
        {
            ColorPrintf(YELLOW, _T("%s启动规范：\t\t\t\t"), leftMargin);
            ColorPrintf(AQUA, _T("EL TORITO\n"));

            /* 输出ValidationEntry中信息 */
            if(JumpToISOValidationEntry(media) == SUCCESS)
            {
                ColorPrintf(WHITE, _T("%s支持平台：\t\t\t\t"), leftMargin);
                ColorPrintf(LIME, _T("%s\n"), GetISOPlatformID(media));
            }

            /* 输出InitialEntry中信息 */
            if(JumpToISOInitialEntry(media) == SUCCESS)
            {
                ColorPrintf(WHITE, _T("%s光盘类型：\t\t\t\t"), leftMargin);
                ColorPrintf(LIME, _T("可启动光盘\n"));

                ColorPrintf(YELLOW, _T("%s启动介质类型：\t\t\t"), leftMargin);
                ColorPrintf(AQUA, _T("%s\n"), GetISOBootMediaType(media));
            }
        }

        _tprintf(_T("\n"));

        retVal = SUCCESS;
    }
    return retVal;
}

int DisplayIMGInfo(media_t media)
{
    int retVal = FAILED;

    media_access access;
    
    if(GetMediaAccess(media, &access, sizeof(BPB)) == SUCCESS)
    {
        const BPB *pcBPB = (const BPB *)access.begin;
        const _TCHAR *fsType = NULL;  //文件系统类型（FAT12/16/32）
        uint32_t totalSec;            //总扇区数
        uint32_t numTrks;             //磁道数
        uint32_t volID = 0;           //卷标
        uint32_t secPerFat = 0;       //每个FAT表所占扇区数
        uint8_t drvNum = 0;           //驱动器号

        /* 计算总扇区数 */
        if(LD_UINT16(pcBPB->Common.BPB_TotSec16))
            totalSec = LD_UINT16(pcBPB->Common.BPB_TotSec16);
        else
            totalSec = LD_UINT32(pcBPB->Common.BPB_TotSec32);

        /* 计算磁道数 */
        numTrks = totalSec / 
            (LD_UINT16(pcBPB->Common.BPB_NumHeads) * LD_UINT16(pcBPB->Common.BPB_SecPerTrk));

        /* 获取卷标、驱动器号、每FAT表扇区数以及文件系统类型 */
        switch(GetIMGType(pcBPB))
        {
        case FAT12:
            volID = LD_UINT32(pcBPB->Special.Fat16.BS_VolID);
            drvNum = LD_UINT8(pcBPB->Special.Fat16.BS_DrvNum);
            secPerFat = LD_UINT16(pcBPB->Common.BPB_FATSz16);
            fsType = _T("FAT12");
            break;
        case FAT16:
            volID = LD_UINT32(pcBPB->Special.Fat16.BS_VolID);
            drvNum = LD_UINT8(pcBPB->Special.Fat16.BS_DrvNum);
            secPerFat = LD_UINT16(pcBPB->Common.BPB_FATSz16);
            fsType = _T("FAT16");
            break;
        case FAT32:
            volID = LD_UINT32(pcBPB->Special.Fat32.BS_VolID);
            drvNum = LD_UINT8(pcBPB->Special.Fat32.BS_DrvNum);
            secPerFat = LD_UINT32(pcBPB->Special.Fat32.BPB_FATSz32);
            fsType = _T("FAT32");
            break;
        }

        ColorPrintf(WHITE, _T("\t系统标名称："));
        ColorPrintfA(LIME, "%.8s", pcBPB->Common.BS_OEMName);

        ColorPrintf(WHITE, _T("\t\t文件系统："));
        ColorPrintf(LIME, _T("%s\n"), fsType);

        ColorPrintf(WHITE, _T("\t卷序列号："));
        ColorPrintf(LIME, _T("0x%.4X"), volID);

        ColorPrintf(WHITE, _T("\t\t介质类型："));
        ColorPrintf(LIME, _T("0x%.1X\n"), LD_UINT8(pcBPB->Common.BPB_Media));

        ColorPrintf(WHITE, _T("\t物理驱动器号："));
        ColorPrintf(LIME, _T("0x%.2X"), drvNum);

        ColorPrintf(WHITE, _T("\t\t每扇区字节数："));
        ColorPrintf(LIME, _T("%u\n"), LD_UINT16(pcBPB->Common.BPB_BytsPerSec));

        ColorPrintf(WHITE, _T("\t隐藏扇区数："));
        ColorPrintf(LIME, _T("%u"), LD_UINT32(pcBPB->Common.BPB_HiddSec));

        ColorPrintf(YELLOW, _T("\t\t\t磁道数（C）："));
        ColorPrintf(AQUA, _T("%u\n"), numTrks);

        ColorPrintf(WHITE, _T("\t每簇扇区数："));
        ColorPrintf(LIME, _T("%u"), LD_UINT8(pcBPB->Common.BPB_SecPerClus));

        ColorPrintf(YELLOW, _T("\t\t\t磁头数（H）："));
        ColorPrintf(AQUA, _T("%u\n"), LD_UINT16(pcBPB->Common.BPB_NumHeads));

        ColorPrintf(WHITE, _T("\t保留扇区数："));
        ColorPrintf(LIME, _T("%u"), LD_UINT16(pcBPB->Common.BPB_RsvdSecCnt));

        ColorPrintf(YELLOW, _T("\t\t\t每磁道扇区数（S）："));
        ColorPrintf(AQUA, _T("%u\n"), LD_UINT16(pcBPB->Common.BPB_SecPerTrk));

        ColorPrintf(WHITE, _T("\tFAT表数："));
        ColorPrintf(LIME, _T("%u"), LD_UINT8(pcBPB->Common.BPB_NumFATs));

        ColorPrintf(WHITE, _T("\t\t\t每FAT表扇区数："));
        ColorPrintf(LIME, _T("%u\n"), secPerFat);

        ColorPrintf(WHITE, _T("\t根目录项数："));
        ColorPrintf(LIME, _T("%u"), LD_UINT16(pcBPB->Common.BPB_RootEntCnt));

        ColorPrintf(FUCHSIA, _T("\t\t\t扇区总数："));
        ColorPrintf(AQUA, _T("%u\n"), totalSec);
    }

    return retVal;
}
