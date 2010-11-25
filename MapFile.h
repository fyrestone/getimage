#ifndef MAP_FILE
#define MAP_FILE

#include "IntTypes.h"

#define MAP_SUCCESS 0
#define MAP_FAILED -1

typedef struct
{
	const void *pvFile;
	uint32_t size;
}File;

//���File�ṹ�����Ч�ԣ���ЧΪ�棬����Ϊ��
#define CHECK_FILE_PTR_IS_VALID(file) \
	((file) && (file)->pvFile && (file)->size)

//���ָ�����һָ����С�����Ƿ���Ч����ЧΪ�棬����Ϊ��
#define CHECK_PTR_SPACE_IS_VALID(file, base, length) \
	(base && (file)->size >= (uint32_t)((char *)(base) + length - (char *)(file)->pvFile))

//���ڳ�����ת����ת�ɹ�����const char *��Ŀ��ָ�룬���򷵻�NULL
#define JUMP_PTR_BY_LENGTH(file, base, length)														\
	(																								\
		(base && (file)->size >= (uint32_t)((char *)(base) + length - (char *)(file)->pvFile)) ?	\
			(char *)(base) + length : NULL/*�������*/												\
	)

int MapFile(File *mapFile, const char *file);

void UnmapFile(const void *pvFile);

#define MEDIA_CUR 0
#define MEDIA_SET 1

#define T media_t

typedef struct T *T;

T OpenMedia(const char *path, uint32_t viewSize);

int RewindMedia(T media);

int SeekMedia(T media, int64_t offset, int base);

void CloseMedia(T *media);

#ifdef _DEBUG
	void MapTestUnit(const char *path);
#endif

#undef T
#endif