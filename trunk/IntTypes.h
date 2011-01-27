/*!
\file IntTypes.h
\author LiuBao
\version 1.0
\date 2011/1/23
\brief 基础数据类型支持
*/
#ifndef INT_TYPES
#define INT_TYPES

typedef char int8_t;                    ///< 8位有符号整型
typedef short int int16_t;              ///< 16位有符号整型
typedef int int32_t;                    ///< 32位有符号整型
typedef long long int64_t;              ///< 64位有符号整型

typedef unsigned char uint8_t;          ///< 8位无符号整型
typedef unsigned short int uint16_t;    ///< 16位无符号整型
typedef unsigned int uint32_t;          ///< 32位无符号整型
typedef unsigned long long uint64_t;    ///< 64位无符号整型

#define LD_UINT8(ptr) ((uint8_t)(*(uint8_t *)(ptr)))    ///< 把指针位置8位长度数据载入为uint8_t
#define LD_UINT16(ptr) ((uint16_t)(*(uint16_t *)(ptr))) ///< 把指针位置16位长度数据载入为uint16_t
#define LD_UINT32(ptr) ((uint32_t)(*(uint32_t *)(ptr))) ///< 把指针位置32位长度数据载入为uint32_t
#define LD_UINT64(ptr) ((uint64_t)(*(uint64_t *)(ptr))) ///< 把指针位置64位长度数据载入为uint64_t

#endif