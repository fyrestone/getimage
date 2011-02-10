/*!
\file MapFile_Test.c
\author LiuBao
\version 1.0
\date 2011/1/29
\brief MapFile�ĵ�Ԫ����
*/

#define WINVER 0x0500       ///< Ϊ��ʹ��GetFileSizeEx

#include <limits.h>
#include <assert.h>
#include <tchar.h>
#include <Windows.h>
#include "MapFile_Test.h"
#include "..\ProjDef.h"     /* ʹ��SUCCESS��FAILED */
#include "..\SafeMemory.h"
#include "..\MapFile.h"
#include "..\AprRing\apr_ring.h"

#define MAP_FILE_PATH _T(".\\data\\MapFile.file")  ///< �������ڲ��Ե�ӳ���ļ�·��
#define OUT_PUT_PATH _T(".\\data\\Output.file")    ///< ����ļ�·��

typedef int MapFileType;                    ///< ӳ���ļ�����

#define LARGER_THAN_ALLOC_GRAN 0x1          ///< �����ڴ��������
#define SMALLER_THAN_ALLOC_GRAN 0x2         ///< С���ڴ��������
#define EQUAL_TO_ALLOC_GRAN 0x3             ///< �����ڴ��������

struct elem_t                               ///< APR��Ԫ�����Ͷ���
{
    APR_RING_ENTRY(elem_t) link;            ///< ������
    media_access *access;                   ///< ������
};

APR_RING_HEAD(elem_head_t, elem_t);         ///< APR��ͷ��㶨��

#define T hack_t                            ///< Ϊ������������T����ʵ����

enum MediaType                              ///< ��������
{
    Map,                                    ///< �ļ�ӳ��
    Raw                                     ///< ֱ��IO
};

struct T                                    ///  ������������media_t����
{
    union                                   ///  ������
    {
        struct _map                         ///  �ļ�ӳ�����ݽṹ
        {
            const _TCHAR *name;             ///< �򿪵Ľ�������·����
            HANDLE hFile;                   ///< �ļ�ӳ����
            PVOID pView;                    ///< ӳ����ͼָ��
            uint32_t allocGran;             ///< �ڴ��������
            uint32_t viewSize;              ///< ��ͼ��С��ӳ��ʱʹ��VIEW��ֱ��IOʱʹ���ض��򻺳�����С
            uint32_t actualViewSize;        ///< ʵ����ͼ��С��ʵ����ͼ��С���ݲ�ͬ�������С�ڵ���viewSize
            uint32_t accessSize;            ///< �ɷ�����ͼ��С������currPos����ͼĩβ�Ĵ�С
            LARGE_INTEGER size;             ///< ���ʴ�С
            LARGE_INTEGER viewPos;          ///< ��ǰ��ͼ����λ�ã���Խ�����ʼλ��ƫ����
            LARGE_INTEGER currPos;          ///< ��ǰ����λ�ã���Խ�����ʼλ��ƫ����
            struct elem_head_t accessRing;  ///< media_accessָ�루��״������ͷ���
        }map;

        struct _raw                         ///  ֱ��IO���ݽṹ
        {
            const _TCHAR *name;             ///< �򿪵Ľ�������·����
            HANDLE hFile;                   ///< ���ʾ��
            PVOID pBuffer;                  ///< ������ָ��
            uint32_t sectorSize;            ///< ����������С
            uint32_t accessSize;            ///< �ɷ�����ͼ��С������currPos����ͼĩβ�Ĵ�С
            LARGE_INTEGER size;             ///< ���ʴ�С
            LARGE_INTEGER currPos;          ///< ��ǰ����λ�ã���Խ�����ʼλ��ƫ����
        }raw;
    };

    enum MediaType type;
};

typedef struct T *T;

/*!
�����ļ�·����ȡ�ļ���С
\param path �ļ�·��
\param lpSize �ļ���Сָ��
\return ��ȡ�ļ���С�ɹ�������SUCCESS�����򷵻�FAILED
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
            GetFileSizeEx(hFile, lpSize);//����ļ���С
            retVal = SUCCESS;
        }

        CloseHandle(hFile);
    }

    return retVal;
}

/*!
��ȡ�ڴ��������
\param lpAllocGran �ڴ�������ȴ�Сָ��
\return ��ȡ�ڴ�������ȳɹ�������SUCCESS�����򷵻�FAILED
*/
static int AllocGran(uint32_t *lpAllocGran)
{
    int retVal = FAILED;

    if(lpAllocGran)
    {
        SYSTEM_INFO sysInfo;

        /* ��ȡ�ڴ�������� */
        GetSystemInfo(&sysInfo);
        *lpAllocGran = sysInfo.dwAllocationGranularity;
        retVal = SUCCESS;
    }

    return retVal;
}

/*!
�����ڴ�������ȼ����������path���ɲ����ļ�
\param path �����ļ�·��
\param allocGran �ڴ��������
\param type �����ļ�����
\return ���ɲ����ļ��ɹ�������SUCCESS�����򷵻�FAILED
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
            case LARGER_THAN_ALLOC_GRAN://���ڴ�������ȴ�
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
            case SMALLER_THAN_ALLOC_GRAN://���ڴ��������С
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
            case EQUAL_TO_ALLOC_GRAN://���ڴ�������ȴ�С��ͬ
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

    /* ���������ļ� */
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

        /* ���media_t״̬ */
        {
            media_t media = OpenMedia(MAP_FILE_PATH, 0);
            hack_t hack = (hack_t)media;

            LCUT_ASSERT(tc, "OpenMedia����NULL", hack != NULL);
            LCUT_ASSERT(tc, "�ļ�ӳ��ʱ��media_t->hFileΪINVALID_HANDLE_VALUE", hack->map.hFile != INVALID_HANDLE_VALUE);
            LCUT_ASSERT(tc, "�ļ�ӳ��ʱ��media_t->pViewΪNULL", hack->map.pView != NULL);
            LCUT_ASSERT(tc, "media_t���ڴ�������ȴ���", hack->map.allocGran == allocGran);
            LCUT_ASSERT(tc, "media_t���ļ���С����", hack->map.size.QuadPart == fileSize.QuadPart);
            LCUT_ASSERT(tc, "media_t�пɷ�����ͼ��СΪ0", hack->map.accessSize != 0);
            LCUT_ASSERT(tc, "media_t�е�ǰλ�ò�Ϊ0", hack->map.currPos.QuadPart == 0);
            LCUT_ASSERT(tc, "media_t�е�ǰ��ͼ��ʼλ�ò�Ϊ0", hack->map.viewPos.QuadPart == 0);

            if(testFileType == LARGER_THAN_ALLOC_GRAN)
                /* �ļ������ڴ�������ȣ�ʹ�÷ֿ�ӳ�䣬��ʱviewSizeӦ������Ϊ1��allocGran */
                LCUT_ASSERT(tc, "media_t->viewSize����", hack->map.viewSize >= allocGran);
            else
                /* �ļ�С�ڵ����ڴ�������ȣ�ʹ����ȫӳ�䣬��ʱviewSizeΪ0 */
                LCUT_ASSERT(tc, "media_t->viewSize��Ϊ0", hack->map.viewSize == 0);

            CloseMedia(&media);
        }

        status = 1;
    }while(++testFileType <= 3);

    /* ���Բ����ļ������ɹ� */
    LCUT_ASSERT(tc, "�����ļ�����ʧ��", status);
}

static void TestMapFileJumpWithFullMap(lcut_tc_t *tc, void *data)
{
    uint32_t allocGran = 0;

    if(AllocGran(&allocGran) == SUCCESS && 
        CreateMapFile(MAP_FILE_PATH, allocGran, SMALLER_THAN_ALLOC_GRAN) == SUCCESS)
    {
        media_t testMedia = OpenMedia(MAP_FILE_PATH, 0);
        hack_t backupMedia;

        LCUT_ASSERT(tc, "��media_tʧ��", testMedia != NULL);
        LCUT_ASSERT(tc, "��������media_tʧ��", NEW(backupMedia) != NULL);

        /* ����testMedia��backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* ������ת���ɷ��ʷ�Χ�߽磬Ӧ��ʧ�� */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.accessSize, MEDIA_SET) != SUCCESS);

        /* ���������ת1��Ӧ��ʧ�� */
        LCUT_TRUE(tc, SeekMedia(testMedia, -1, MEDIA_SET) != SUCCESS);

        /* ��תʧ��ʱ��media_t״̬Ӧ������ */
        LCUT_TRUE(tc, !memcmp(backupMedia, testMedia, sizeof(*backupMedia)));

        /* ������ת���ɷ��ʷ�Χ�߽�-1��Ӧ���ɹ� */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.accessSize - 1, MEDIA_SET) == SUCCESS);

        /* ����media */
        LCUT_TRUE(tc, RewindMedia(testMedia) == SUCCESS);

        /* ������ú�media_tӦ����ԭʼmedia_t���ݣ�backupMedia����ͬ */
        LCUT_TRUE(tc, !memcmp(backupMedia, testMedia, sizeof(*backupMedia)));

        /* �ٴ���ת���ɷ��ʷ�Χ�߽�-1��Ӧ���ɹ� */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.accessSize - 1, MEDIA_SET) == SUCCESS);

        /* ���������ת���ɷ��ʷ�Χ�߽�-1������ */
        LCUT_TRUE(tc, SeekMedia(testMedia, -(int64_t)(backupMedia->map.actualViewSize - 1), MEDIA_CUR) == SUCCESS);
    
        /* ��ʱӦ���س�ʼλ�ã�media_tӦ����ԭʼmedia_t���ݣ�backupMedia����ͬ */
        LCUT_TRUE(tc, !memcmp(backupMedia, testMedia, sizeof(*backupMedia)));

        /* �ٴ���ת���ɷ��ʷ�Χ�߽�-1��Ӧ���ɹ� */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.accessSize - 1, MEDIA_SET) == SUCCESS);

        /* ���ݵ�ǰ״̬��testMedia��backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* ���������ת���ɷ��ʷ�Χ�߽磬Ӧ��ʧ�� */
        LCUT_TRUE(tc, SeekMedia(testMedia, -(int64_t)(backupMedia->map.actualViewSize), MEDIA_CUR) != SUCCESS);

        /* ������أ��ɷ��ʷ�Χ�߽�-1��/2 */
        LCUT_TRUE(tc, SeekMedia(testMedia, -(int64_t)(backupMedia->map.actualViewSize - 1)/2, MEDIA_CUR) == SUCCESS);

        /* ��ǰ���أ��ɷ��ʷ�Χ�߽�-1��/2 */
        LCUT_TRUE(tc, SeekMedia(testMedia, (int64_t)(backupMedia->map.actualViewSize - 1)/2, MEDIA_CUR) == SUCCESS);
    
        /* �����ת��media_tӦ����media_t���ݣ�backupMedia����ͬ */
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

        LCUT_ASSERT(tc, "��media_tʧ��", testMedia != NULL);
        LCUT_ASSERT(tc, "��������media_tʧ��", NEW(backupMedia) != NULL);

        /* ����testMedia��backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* �ֲ���ת��Ӧ�ò����л�ӳ����ͼ */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.allocGran - 1, MEDIA_SET) == SUCCESS);

        /* ���ֲ���ת����ͼ��Ȼ��ԭ����ͼ */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.pView == backupMedia->map.pView);

        /* ���ֲ���ת����ͼλ����Ȼ��ԭ��λ�� */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.viewPos.QuadPart == backupMedia->map.viewPos.QuadPart);

        /* ���ֲ���ת��ǰλ��Ӧ������ͼλ�ò�ͬ */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.currPos.QuadPart != ((hack_t)testMedia)->map.viewPos.QuadPart);

        /* ����media */
        LCUT_TRUE(tc, RewindMedia(testMedia) == SUCCESS);

        /* ������ú�media_tӦ����ԭʼmedia_t���ݣ�backupMedia����ͬ */
        LCUT_TRUE(tc, !memcmp(testMedia, backupMedia, sizeof(*backupMedia)));

        /* ��תһ���ڴ����ȣ�Ӧ�øպ��л���ͼ */
        LCUT_TRUE(tc, SeekMedia(testMedia, backupMedia->map.allocGran, MEDIA_SET) == SUCCESS);

        /* �����ת����ͼ�Ƿ����л� */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.pView != backupMedia->map.pView);

        /* ������תλ�øպ��л���ͼ����ת��ǰλ��Ӧ������ͼλ����ͬ */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.currPos.QuadPart == ((hack_t)testMedia)->map.viewPos.QuadPart);

        /* ������תλ�øպ��л���ͼ����ת��ɷ��ʴ�С������ʵ����ͼ��С */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.accessSize == ((hack_t)testMedia)->map.actualViewSize);

        /* ��ת����ͼ��СӦ������ */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.viewSize == backupMedia->map.viewSize);

        /* ������תλ�þ����ļ�ĩβ����С����ͼ��С����ת��ʵ����ͼ��СӦ��С����ͼ��С */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.actualViewSize < ((hack_t)testMedia)->map.viewSize);

        /* ������ת���media_t��backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* �����ת1��Ӧ���л���ͼ */
        LCUT_TRUE(tc, SeekMedia(testMedia, -1, MEDIA_CUR) == SUCCESS);

        /* �����ת����ͼ�Ƿ����л� */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.pView != backupMedia->map.pView);

        /* ������ת���media_t��backupMedia */
        LCUT_TRUE(tc, memcpy(backupMedia, testMedia, sizeof(*backupMedia)) != NULL);

        /* ��ʱ�ӵ�ǰλ�������ת(һ���ڴ����� - 1) */
        LCUT_TRUE(tc, SeekMedia(testMedia, -(int64_t)(((hack_t)testMedia)->map.allocGran - 1), MEDIA_CUR) == SUCCESS);

        /* ��ʱ�൱��RewindMedia����ͼ��ʼλ��Ӧ��Ϊ0 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.viewPos.QuadPart == 0);

        /* ��ʱ�൱��RewindMedia�󣬵�ǰλ��Ӧ��Ϊ0 */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.currPos.QuadPart == 0);

        /* ʵ����ͼ��СӦ������ͼ��С��ͬ */
        LCUT_TRUE(tc, ((hack_t)testMedia)->map.actualViewSize == ((hack_t)testMedia)->map.viewSize);

        /* ����ǰ����һ���ڴ����� */
        LCUT_TRUE(tc, SeekMedia(testMedia, ((hack_t)testMedia)->map.allocGran - 1, MEDIA_SET) == SUCCESS);

        /* ���media���ϴα��ݵ�backupMeida��ͬ */
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

        LCUT_ASSERT(tc, "OpenMediaʧ��", testMedia != NULL);
        LCUT_ASSERT(tc, "fopenʧ��", fp != NULL);

        SeekMedia(testMedia, 10, MEDIA_CUR);
        DumpMedia(testMedia, fp, ((hack_t)testMedia)->map.size.QuadPart - 10);
        CloseMedia(&testMedia);
    }
}

lcut_ts_t *CreateMapFileTestSuite()
{
    lcut_ts_t *suite = NULL;
    int _cut_status;

    LCUT_TS_INIT(suite, "MapFile ��Ԫ�����׼�", NULL, NULL);

    LCUT_TC_ADD(suite, "�����ļ�ӳ����ļ�", TestMapFileOpen, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "������ȫ�ļ�ӳ����ת", TestMapFileJumpWithFullMap, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "���Էֿ��ļ�ӳ����ת", TestMapFileJumpWithSplitMap, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "����ֱ��IO", TestDirectIODevice, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "����ӳ���ļ�����", TestMapFileAccess, NULL, NULL, NULL);
    LCUT_TC_ADD(suite, "����ӳ���ļ���ȡ", TestMapFileDump, NULL, NULL, NULL);

    return suite;
}
