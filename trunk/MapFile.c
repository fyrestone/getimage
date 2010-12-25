#define WINVER 0x0500	/*!< Ϊ��ʹ��GetFileSizeEx */

#include <Windows.h>
#include <io.h>			/*!< _access������ļ����� */
#include <assert.h>
#include "ProjDef.h"
#include "MapFile.h"
#include "SafeMemory.h"

#define T media_t

struct T
{
	const char *name;			/*!< �򿪵Ľ�������·���� */
	HANDLE hFile;				/*!< ӳ�����ʱʹ�õľ�� */
	FILE *pFile;				/*!< ֱ��IOʱʹ�õĽṹ�� */
	PVOID pView;				/*!< ӳ�����ʱ����ͼָ�� */
	uint32_t allocGran;			/*!< �ڴ�������� */
	uint32_t viewSize;			/*!< ��ͼ��С��ӳ��ʱʹ��VIEW��ֱ��IOʱʹ���ض��򻺳�����С */
	uint32_t actualViewSize;	/*!< ʵ����ͼ��С��ʵ����ͼ��С���ݲ�ͬ�������С�ڵ���viewSize */
	uint32_t accessSize;		/*!< �ɷ�����ͼ��С������currPos����ͼĩβ�Ĵ�С */
	LARGE_INTEGER size;			/*!< ���ʴ�С */
	LARGE_INTEGER viewPos;		/*!< ��ǰ��ͼ����λ�ã���Խ�����ʼλ��ƫ���� */
	LARGE_INTEGER currPos;		/*!< ��ǰ����λ�ã���Խ�����ʼλ��ƫ���� */
};

static HANDLE MapMedia(const char *path, PLARGE_INTEGER lpFileSize);
static int SplitMapView(T media, int64_t offset);
static int FullMapView(T media, uint32_t offset);
static int SeekMapMedia(T media, int64_t offset, int base);
static int SeekRawMedia(T media, int64_t offset, int base);
static T OpenMapMedia(T media, const char *path, uint32_t viewSize);

static HANDLE MapMedia(const char *path, PLARGE_INTEGER lpFileSize)
{
	HANDLE hFileMapping = NULL;

	if(path && lpFileSize)
	{
		HANDLE hFile = CreateFileA(
			path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if(hFile != INVALID_HANDLE_VALUE)
		{
			if(GetFileSizeEx(hFile, lpFileSize))//����ļ���С
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
#define ALIGN_COST (offset & (media->allocGran - 1)) /*!< �������ĵĴ�С */

	int retVal = FAILED;

	LARGE_INTEGER alignedOffset;

	assert((offset & ~(media->allocGran - 1)) == (offset / media->allocGran) * media->allocGran);

	/*! ƫ�������ڴ����ȶ��� */
	alignedOffset.QuadPart = offset & ~(media->allocGran - 1);

	if(media->pView && alignedOffset.QuadPart == media->viewPos.QuadPart)
	{
		media->currPos.QuadPart = offset;
		media->accessSize = media->actualViewSize - ALIGN_COST;

		return SUCCESS;
	}
	else//��������ƫ�����뵱ǰ��ͼλ�ò�ͬ�����߻�δӳ��
	{
		/*!
			ÿ����תλ��offset������ǰ��ͼλ��viewPos��
			һ���ڴ��������allocGranʱ��offset����λ��
			alignedOffset������תǰ����ͼλ��viewPos��ͬ��
			����ִ�д˶�else���롣

			�˶�else��������λ��alignedOffsetӳ������ͼ��
			�ɹ�������ԭ����ͼ������media�ṹֵ��
		*/
		uint32_t actualViewSize;
		PVOID pNewView;

		/*! ����ʵ����ͼ��СactualViewSize */
		if(alignedOffset.QuadPart + media->viewSize > media->size.QuadPart)
			actualViewSize = (uint32_t)(media->size.QuadPart - alignedOffset.QuadPart);
		else
			actualViewSize = media->viewSize;

		pNewView = MapViewOfFile(
			media->hFile, FILE_MAP_READ, alignedOffset.HighPart, alignedOffset.LowPart, actualViewSize);

		if(pNewView)
		{
			/*! ��media���ھ�ӳ����ͼ,�������ٳɹ�,����media�����ھ�ӳ����ͼ */
			if((media->pView && UnmapViewOfFile(media->pView)) || !media->pView)
			{
				media->pView = pNewView;
				media->viewPos = alignedOffset;
				media->actualViewSize = actualViewSize;

				/*!
					�ɷ�����ͼ��СӦ������ʵ�ʵ���ͼ��СactualViewSize��ȥ
					����offsetʱ���ĵĴ�С����offset-alignedOffset��
					�����Ż�һ��ʹ��λ��������ٶȡ�
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

	if(media->pView)//����Ѿ�ӳ��ɹ�
	{
		/*! 
			offset�Ѿ��Ǿ���ƫ����������ɷ�����ͼ��СaccessSize
			ֱ����ʵ����ͼ��С��ͬ��Ҳ�ǽ��ʴ�С����ȥoffset����
		*/
		media->currPos.QuadPart = offset;
		media->accessSize = media->actualViewSize - offset;

		retVal = SUCCESS;
	}
	else//�����δӳ�䣬�������ȫӳ��
	{
		PVOID pNewView = MapViewOfFile(
			media->hFile, FILE_MAP_READ, 0, 0, 0);

		if(pNewView)
		{
			media->pView = pNewView;

			/*! 
				�״Σ�Ҳ��Ψһһ�Σ�������ȫӳ���
				�ɷ�����ͼ��С��ʵ����ͼ��СӦ�������
				��С��ͬ
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

		/*! ����offsetΪ����ƫ���� */
		switch(base)
		{
		case MEDIA_SET:
			mendedOffset.QuadPart = offset;
			break;
		case MEDIA_CUR:
			mendedOffset.QuadPart = offset + media->currPos.QuadPart;
			break;
		}

		/*! ���������ƫ����mendedOffsetû�г����ļ���Χ */
		if(mendedOffset.QuadPart >= 0 && mendedOffset.QuadPart < media->size.QuadPart)
		{
			if(media->viewSize)//����ֿ�ӳ��
				retVal = SplitMapView(media, mendedOffset.QuadPart);
			else//�����ȫӳ��
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
		assert(0);
		//TODO:ʹ��fopen��
	}

	return retVal;
}

static T OpenMapMedia(T media, const char *path, uint32_t viewSize)
{
	T retVal = NULL;

	if(media && path)
	{
		LARGE_INTEGER fileSize;
		HANDLE hFileMapping = MapMedia(path, &fileSize);

		if(hFileMapping)
		{
			SYSTEM_INFO sysInfo;
			
			/*! ��ȡ�ڴ�������� */
			GetSystemInfo(&sysInfo);

			if(sysInfo.dwAllocationGranularity)
			{
				uint32_t roundViewSize = /*! �϶������ڴ����� */
					(viewSize & ~(sysInfo.dwAllocationGranularity - 1)) + sysInfo.dwAllocationGranularity;
					
				assert(((viewSize / sysInfo.dwAllocationGranularity) + 1) * sysInfo.dwAllocationGranularity
					== roundViewSize);

				if(roundViewSize >= viewSize)//��ֹ�϶��������
				{
					viewSize = roundViewSize;

					if(!fileSize.HighPart && viewSize >= fileSize.LowPart)//viewSize����fileSize
						viewSize = 0;

					/*! ��ʼ��media�ṹ�� */
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
		if(media->hFile)//�����ӳ���ļ�
			retVal = SeekMapMedia(media, offset, base);
		else if(media->pFile)//�����fopen�򿪽���
			retVal = SeekRawMedia(media, offset, base);
	}

	return retVal;
}

T OpenMedia(const char *path, uint32_t viewSize)
{
	T media, retVal = NULL;

	if(path && NEW(media))
	{
		if(_access(path, 0/* ������ */))//����ļ�������
		{
			assert(0);
		}
		else//����ļ�����
			retVal = OpenMapMedia(media, path, viewSize);
	}

	return retVal;
}

void CloseMedia(T *media)
{
	T mediaCopy = *media;

	if(media && mediaCopy)
	{
		if(mediaCopy->hFile)//�����ӳ���ļ�
		{
			UnmapViewOfFile(mediaCopy->pView);	/*!< ж����ͼ */
			CloseHandle(mediaCopy->hFile);		/*!< �ر��ļ���� */
			FREE(mediaCopy);					/*!< �ͷ�media_t�ṹ */
			*media = NULL;						/*!< ����ָ���media_t��NULL */
		}
		else if(mediaCopy->pFile)//�����fopen�򿪽���
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
				assert(0);
				//TODO:��̬�����ڴ�ռ�
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

		if(media->hFile)//������ļ�ӳ��
		{
			int hasError = 0;

			while(size > media->accessSize)
			{
				const unsigned char *writePtr = 
					(const unsigned char *)media->pView + media->viewSize - media->accessSize;
				uint32_t writeLen;//д�볤��

				if(fwrite(writePtr, media->accessSize, 1, fp) != 1)
				{
					hasError = 1;
					break;
				}

				writeLen = media->accessSize;//����д��ĳ���
				
				/* �����ת�����޸�media->accessSize */
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
			assert(0);
			//TODO:
		}

		if(SeekMedia(media, currPos.QuadPart, MEDIA_SET) != SUCCESS)
			retVal = FAILED;

#ifdef _DEBUG
		assert(!memcmp(media_bak, media, sizeof(*media)));
#endif
	}

	return retVal;
}

//========================���Դ��뿪ʼ=============================

#ifdef _DEBUG

#include "ColorPrint.h"	/*!< ��������ʹ�� */

void MapTestUnit(const char *path)
{
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

	LARGE_INTEGER fileSize;
	uint32_t allocGran;

	fileSize.QuadPart = 0;//��ʼ��0
	allocGran = 0;

	/*! ����ļ���СfileSize */
	{
		HANDLE hFile = CreateFileA(
			path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if(hFile != INVALID_HANDLE_VALUE)
			GetFileSizeEx(hFile, &fileSize);//����ļ���С

		CloseHandle(hFile);
	}

	/*! ����ڴ�������ȴ�СallocGran */
	{
		SYSTEM_INFO sysInfo;

		/*! ��ȡ�ڴ�������� */
		GetSystemInfo(&sysInfo);
		allocGran = sysInfo.dwAllocationGranularity;
	}


	{
		uint32_t viewSize;

		ColorPrintf(WHITE, "���ڲ����ļ���\t\t\t");
		assert(fileSize.QuadPart);
		ColorPrintf(GREEN, "ͨ��\n");

		ColorPrintf(WHITE, "���ڲ��Ի�ȡ�ڴ��������\t\t");
		assert(allocGran);
		ColorPrintf(GREEN, "ͨ��\n");

		ColorPrintf(YELLOW, "=>�ļ���С��%I64d\t�ڴ�������ȣ�\%u\n", fileSize.QuadPart, allocGran);

		ColorPrintf(SILVER, "->ȡ��ͼ >= �ļ�����������ȫӳ�䣺\n");
		{
			LARGE_INTEGER tempViewSize;

			tempViewSize.QuadPart = MAX(fileSize.QuadPart, allocGran) + 10;

			if(tempViewSize.HighPart)
				ColorPrintf(RED, "�ļ���С������ͼ���ޣ�������⣡\n");
			else
			{
				T testMedia, backupMedia = NULL;

				viewSize = tempViewSize.LowPart;

				ColorPrintf(WHITE, "���ڲ���ӳ���ļ�\t\t\t");
				testMedia = OpenMedia(path, viewSize);
				assert(testMedia->allocGran == allocGran);					/*!< ���������ȷ */
				assert(!testMedia->currPos.QuadPart);						/*!< ��鵱ǰλ��Ϊ0 */
				assert(testMedia->hFile);									/*!< ����ļ�������� */
				assert(!memcmp(testMedia->name, path, strlen(path) + 1));	/*!< ���������ȷ */
				assert(testMedia->pView);									/*!< �����ͼָ��Ӧ����Ϊ�� */
				assert(!testMedia->pFile);									/*!< ����ļ�ָ��Ӧ��Ϊ�� */
				assert(testMedia->size.QuadPart == fileSize.QuadPart);		/*!< ����ļ���С��ȷ */
				assert(testMedia->accessSize);								/*!< ���ɷ�����ͼ��СӦ����Ϊ0 */
				assert(!testMedia->viewPos.QuadPart);						/*!< �����ͼ��ʼλ��Ϊ0 */
				assert(!testMedia->viewSize);								/*!< �����ͼ��СӦ��Ϊ0 */
				assert(!(testMedia->viewPos.QuadPart % allocGran));			/*!< �����ͼ��ʼλ�ö����ڴ����� */
				ColorPrintf(GREEN, "ͨ��\n");

				ColorPrintf(WHITE, "���ڲ�����ת\t\t\t\t");
				assert(NEW(backupMedia));																				/*!< ����backupMedia */
				assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));												/*!< ����media��backupMedia */
				assert(SeekMedia(testMedia, testMedia->accessSize, MEDIA_SET) != SUCCESS);							/*!< ������ת���ɷ��ʷ�Χ�߽磬Ӧ��ʧ�� */
				assert(!memcmp(backupMedia, testMedia, sizeof(*testMedia)));											/*!< ��תʧ��ʱ��mediaֵӦ������ */
				assert(SeekMedia(testMedia, testMedia->accessSize - 1, MEDIA_SET) == SUCCESS);						/*!< ������ת���ɷ��ʷ�Χ�߽�-1��Ӧ���ɹ� */
				assert(RewindMedia(testMedia) == SUCCESS);															/*!< ����media */
				assert(!memcmp(backupMedia, testMedia, sizeof(*testMedia)));											/*!< ������ú�mediaӦ����ԭʼmedia��ͬ */
				assert(SeekMedia(testMedia, testMedia->accessSize - 1, MEDIA_SET) == SUCCESS);						/*!< ������ת���ɷ��ʷ�Χ�߽�-1��Ӧ���ɹ� */
				assert(SeekMedia(testMedia, -(int64_t)(testMedia->actualViewSize - 1), MEDIA_CUR) == SUCCESS);		/*!< �ӵ�ǰ�������ͬ������ */
				assert(!memcmp(backupMedia, testMedia, sizeof(*testMedia)));											/*!< ������ú�mediaӦ����ԭʼmedia��ͬ */
				assert(SeekMedia(testMedia, testMedia->accessSize - 1, MEDIA_SET) == SUCCESS);						/*!< ������ת���ɷ��ʷ�Χ�߽�-1��Ӧ���ɹ� */
				assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));												/*!< ���ݵ�ǰmedia��backupMedia */
				assert(SeekMedia(testMedia, -(int64_t)(testMedia->actualViewSize - 1)/2, MEDIA_CUR) == SUCCESS);	/*!< ������أ��ɷ��ʷ�Χ�߽�-1��/2 */
				assert(SeekMedia(testMedia, (int64_t)(testMedia->actualViewSize - 1)/2, MEDIA_CUR) == SUCCESS);		/*!< ��ǰ���أ��ɷ��ʷ�Χ�߽�-1��/2 */
				assert(!memcmp(backupMedia, testMedia, sizeof(*testMedia)));											/*!< ������غ�mediaӦ�����ϴα��ݵ�backupMedia��ͬ */
				ColorPrintf(GREEN, "ͨ��\n");

				CloseMedia(&testMedia);
			}
		}

		ColorPrintf(SILVER, "->ȡ��ͼ < �ļ��������зֿ�ӳ�䣺\n");
		{
			LARGE_INTEGER tempViewSize;

			tempViewSize.QuadPart = fileSize.QuadPart - allocGran - 1;

			if(tempViewSize.HighPart)
				ColorPrintf(RED, "�ļ���С������ͼ���ޣ�������⣡\n");
			else if(tempViewSize.QuadPart <= allocGran)
				ColorPrintf(RED, "�ļ���С�ӽ��ڴ�������ȣ�������⣡\n");
			else if((tempViewSize.LowPart & ~(allocGran - 1)) + allocGran >= fileSize.QuadPart)
				ColorPrintf(RED, "��ͼ��С�϶����ڴ�������Ⱥ� >= �ļ���С��������⣡\n");
			else
			{
				T testMedia, backupMedia = NULL;

				viewSize = tempViewSize.LowPart;

				ColorPrintf(WHITE, "���ڲ���ӳ���ļ�\t\t\t");
				testMedia = OpenMedia(path, viewSize);
				assert(testMedia->allocGran == allocGran);					/*!< ���������ȷ */
				assert(!testMedia->currPos.QuadPart);						/*!< ��鵱ǰλ��Ϊ0 */
				assert(testMedia->hFile);									/*!< ����ļ�������� */
				assert(!memcmp(testMedia->name, path, strlen(path) + 1));	/*!< ���������ȷ */
				assert(testMedia->pView);									/*!< �����ͼָ��Ӧ����Ϊ�� */
				assert(!testMedia->pFile);									/*!< ����ļ�ָ��Ӧ��Ϊ�� */
				assert(testMedia->size.QuadPart == fileSize.QuadPart);		/*!< ����ļ���С��ȷ */
				assert(testMedia->accessSize);								/*!< ���ɷ�����ͼ��СӦ����Ϊ0 */
				assert(!testMedia->viewPos.QuadPart);						/*!< �����ͼ��ʼλ��Ϊ0 */
				assert(testMedia->viewSize);								/*!< �����ͼ��СӦ����Ϊ0 */
				assert(!(testMedia->viewPos.QuadPart % allocGran));			/*!< �����ͼ��ʼλ�ö����ڴ����� */
				assert(testMedia->viewSize >= viewSize);					/*!< �������϶����ʵ�ʵ�viewSizeӦ�����ڵ��ڴ���viewSize */
				ColorPrintf(GREEN, "ͨ��\n");

				ColorPrintf(WHITE, "���ڲ�����ת\t\t\t\t");
				assert(NEW(backupMedia));																				/*!< ����backupMedia */
				assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));												/*!< ����media��backupMedia */
				assert(SeekMedia(testMedia, testMedia->allocGran - 1, MEDIA_SET) == SUCCESS);						/*!< �ֲ���ת��Ӧ�ò����л�ӳ����ͼ */
				assert(testMedia->pView == backupMedia->pView);															/*!< ���ֲ���ת����ͼ��Ȼ��ԭ����ͼ */
				assert(testMedia->viewPos.QuadPart == backupMedia->viewPos.QuadPart);									/*!< ���ֲ���ת����ͼλ����Ȼ��ԭ��λ�� */
				assert(testMedia->currPos.QuadPart != testMedia->viewPos.QuadPart);										/*!< ���ֲ���ת��ǰλ��Ӧ������ͼλ�ò�ͬ */
				assert(RewindMedia(testMedia) == SUCCESS);															/*!< ����media */
				assert(!memcmp(testMedia, backupMedia, sizeof(*testMedia)));											/*!< ������ú��media����ǰ���ݵ���ͬ */
				assert(SeekMedia(testMedia, testMedia->allocGran, MEDIA_SET) == SUCCESS);							/*!< ��תһ���ڴ����ȣ�Ӧ�øպ��л���ͼ */
				assert(testMedia->pView != backupMedia->pView);															/*!< �����ת����ͼ�Ƿ����л� */
				assert(testMedia->currPos.QuadPart == testMedia->viewPos.QuadPart);										/*!< ������תλ�øպ��л���ͼ����ת��ǰλ��Ӧ������ͼλ����ͬ */
				assert(testMedia->accessSize == testMedia->actualViewSize);												/*!< ������תλ�øպ��л���ͼ����ת��ɷ��ʴ�С������ʵ����ͼ��С */
				assert(testMedia->viewSize == backupMedia->viewSize);													/*!< ��ת����ͼ��СӦ������ */
				assert(testMedia->actualViewSize < backupMedia->actualViewSize);										/*!< ������תλ�þ����ļ�ĩβ����С����ͼ��С����ת��ʵ����ͼ��СӦ��С����ͼ��С */
				assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));												/*!< ������ת���media��backupMedia */
				assert(SeekMedia(testMedia, -1, MEDIA_CUR) == SUCCESS);												/*!< �����ת1��Ӧ���л���ͼ */
				assert(testMedia->pView != backupMedia->pView);															/*!< �����ת����ͼ�Ƿ����л� */
				assert(memcpy(backupMedia, testMedia, sizeof(*testMedia)));												/*!< ������ת���media��backupMedia */
				assert(SeekMedia(testMedia, -(int64_t)(testMedia->allocGran - 1), MEDIA_CUR) == SUCCESS);			/*!< ��ʱ�ӵ�ǰλ�������תһ���ڴ����� */
				assert(testMedia->viewPos.QuadPart == 0);																/*!< ��ʱ�൱��RewindMedia����ͼλ��Ӧ��Ϊ0 */
				assert(testMedia->currPos.QuadPart == 0);																/*!< ��ʱ�൱��RewindMedia�󣬵�ǰλ��Ӧ��Ϊ0 */
				assert(testMedia->actualViewSize == testMedia->viewSize);												/*!< ʵ����ͼ��СӦ������ͼ��С��ͬ */
				assert(SeekMedia(testMedia, testMedia->allocGran - 1, MEDIA_SET) == SUCCESS);						/*!< ����ǰ����һ���ڴ����� */
				assert(!memcmp(testMedia, backupMedia, sizeof(*testMedia)));											/*!< ���media���ϴα��ݵ�backupMeida��ͬ */
				ColorPrintf(GREEN, "ͨ��\n");

				CloseMedia(&testMedia);
			}
		}

		/*! ������� */
		ColorPrintf(GREEN, "ȫ������ͨ����\n");
	}

#undef MIN
#undef MAX
}

#endif

//========================���Դ������=============================