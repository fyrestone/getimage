/*!
\file Process.h
\author LiuBao
\version 1.0
\date 2011/1/23
\brief 声明处理函数
*/
#ifndef PROCESS
#define PROCESS

#include "MapFile.h"

typedef int MEDIA_TYPE;	///< 介质格式定义

#define UNKNOWN 0x0		///< 任何未知类型
#define IMG 0x1			///< IMG格式
#define ISO 0x2			///< ISO格式	

/*!
获得介质类型，可能是UNKNOWN、IMG、ISO
\param media 位置在介质头部的media_t
\return 介质类型（UNKNOWN、IMG、ISO）
*/
MEDIA_TYPE GetInputType(media_t media);

/*!
从ISO中提取出启动IMG映像，保存在ISO文件所在路径，扩展名为.img
\param media 位置在ISO头部的media_t
\param path ISO文件绝对路径
\return 提取成功返回SUCCESS；否则返回FAILED
*/
int DumpIMGFromISO(media_t media, const char *path);

/*!
输出ISO规格信息
\param media 位置在ISO头部的media_t
\return 成功返回SUCCESS；否则返回FAILED
*/
int DisplayISOInfo(media_t media);

/*!
输出IMG规格信息
\param media 位置在IMG头部的media_t
\return 成功返回SUCCESS；否则返回FAILED
*/
int DisplayIMGInfo(media_t media);

#endif