/*!
\file IntTypes.h
\author LiuBao
\version 1.0
\date 2011/1/23
\brief ������������֧��
*/
#ifndef INT_TYPES
#define INT_TYPES

typedef char int8_t;					///< 8λ�з�������
typedef short int int16_t;				///< 16λ�з�������
typedef int int32_t;					///< 32λ�з�������
typedef long long int64_t;				///< 64λ�з�������

typedef unsigned char uint8_t;			///< 8λ�޷�������
typedef unsigned short int uint16_t;	///< 16λ�޷�������
typedef unsigned int uint32_t;			///< 32λ�޷�������
typedef unsigned long long uint64_t;	///< 64λ�޷�������

#define LD_UINT8(ptr) ((uint8_t)(*(uint8_t *)(ptr)))	///< ��ָ��λ��8λ������������Ϊuint8_t
#define LD_UINT16(ptr) ((uint16_t)(*(uint16_t *)(ptr)))	///< ��ָ��λ��16λ������������Ϊuint16_t
#define LD_UINT32(ptr) ((uint32_t)(*(uint32_t *)(ptr)))	///< ��ָ��λ��32λ������������Ϊuint32_t
#define LD_UINT64(ptr) ((uint64_t)(*(uint64_t *)(ptr)))	///< ��ָ��λ��64λ������������Ϊuint64_t

#endif