#ifndef INT_TYPES
#define INT_TYPES

typedef char int8_t;
typedef short int int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

/*！ 未启用的数据类型
typedef union _LARGE_INTEGER {
  struct {
    uint32_t LowPart;
    int32_t  HighPart;
  } ;
  struct {
    uint32_t LowPart;
    int32_t  HighPart;
  } u;
  int64_t QuadPart;
} LARGE_INTEGER;

typedef union _LARGE_INTEGER {
    struct {
        uint32_t LowPart;
        uint32_t HighPart;
    } ;
    struct {
        uint32_t LowPart;
        uint32_t HighPart;
    } u;
    uint64_t QuadPart;
} ULARGE_INTEGER;
*/

#define LD_UINT8(ptr) ((uint8_t)(*(uint8_t *)(ptr)))
#define LD_UINT16(ptr) ((uint16_t)(*(uint16_t *)(ptr)))
#define LD_UINT32(ptr) ((uint32_t)(*(uint32_t *)(ptr)))
#define LD_UINT64(ptr) ((uint64_t)(*(uint64_t *)(ptr)))

#endif