﻿/*!
\file MapFile.h
\author LiuBao
\version 1.0
\date 2010/12/28
\brief 通用介质操作接口，支持文件映射操作
*/
#ifndef MAP_FILE
#define MAP_FILE

#include <stdio.h>        /* FILE */
#include <tchar.h>        /* 使用_TCHAR */
#include "IntTypes.h"

#define MEDIA_CUR 0       ///< 当前位置
#define MEDIA_SET 1       ///< 起始位置

#define T media_t         ///< 抽象数据类型（ADT）media_t

typedef struct T *T;

typedef struct media_access      ///  介质访问结构体，由GetMediaAccess获得
{
    const unsigned char *begin;  ///< 介质访问指针
    uint32_t len;                ///< 可访问介质大小
}media_access;

/*!
打开介质，优先使用内存映射
\param path 介质路径
\param viewSize 视图大小
\return 若成功，返回有效media_t；否则返回NULL
*/
T OpenMedia(const _TCHAR *path, uint32_t viewSize);

/*!
介质打开位置复位
\param media 需要复位的media_t
\return 成功返回SUCCESS；否则返回FAILED
*/
int RewindMedia(T media);

/*!
跳转介质打开位置
\param media 需要跳转的media_t
\param offset 跳转偏移量，可以为正（向前跳转）或负（向后跳转）
\param base 可能取值为MEDIA_CUR（相对于当前位置）和MEDIA_SET（相对于起始位置）
\return 成功返回SUCCESS；否则返回FAILED
*/
int SeekMedia(T media, int64_t offset, int base);

/*!
关闭介质
\param media 需要关闭的media_t的指针
*/
void CloseMedia(T *media);

/*!
从当前media_t位置获得指定长度的内存块只读指针
\param media 需要获取访问结构的media_t
\param access 访问结构体指针
\param len 从当前位置向后可访问长度
\return 成功获取返回SUCCESS；否则返回FAILED
\warning 若当前位置向后偏移len超出介质，函数调用失败
*/
int GetMediaAccess(T media, media_access *access, uint32_t len);

/*!
从当前media_t位置抽取size大小输出到fp上
\param media 需要抽取数据的media_t
\param fp 输出指针，可以stdout或者可写的文件指针
\param size 抽取大小
\return 抽取成功返回SUCCESS；否则返回FAILED
\warning 若size <= 0，函数调用失败\n若当前位置向后偏移size超出介质，函数调用失败
*/
int DumpMedia(T media, FILE *fp, int64_t size);

#undef T
#endif