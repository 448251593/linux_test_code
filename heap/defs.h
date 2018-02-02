
#ifndef __DEFS_H__
#define __DEFS_H__

#include <stddef.h>
#include "string_utils.h"

#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

__BEGIN_DECLS

// min，max宏
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

// bool
typedef int BOOL;

#define DBG(...) printf(__VA_ARGS__);
#define ERR(...) fprintf(stderr, __VA_ARGS__);

#define timevalGreaterEqual(a, b)                                              \
  ((a.tv_sec > b.tv_sec) || (a.tv_sec == b.tv_sec && a.tv_usec >= b.tv_usec))

//----------------------------------------------------------------------------
// 常用code
//----------------------------------------------------------------------------
#define CODE_SUCCESS             0
#define CODE_FAILED             -1  //
#define CODE_INVALID_PARAM      -2  // 无效参数
#define CODE_MALLOC_FAILED      -3  // 内存分配失败
#define CODE_NOT_FOUND          -4  // 资源未找到
#define CODE_READ_FAILED        -5  // 读取失败
#define CODE_WRITE_FAILED       -6  // 写失败
#define CODE_OPEN_FAILED        -7  // 打开失败
#define CODE_TIME_OUT           -8  // 超时
#define CODE_INIT_FAILED        -9  // 初始化错误
#define CODE_RES_IN_USED        -10 // 资源已被占用

#define CODE_MAX_CAPACITY_REACH -20 // 达到最大容量
#define CODE_BUFFER_FULL        -21 // 缓冲区已满
#define CODE_CHANNEL_DOWN       -22 // 信道失败


#define CODE_NO_MORE_ELEMENT    -100001 // No element in iterator

//----------------------------------------------------------------------------
//  @brief: 比较函数: 比较函数需实现判断2个给定的元素的大小。
//  @elem1: 需要比较的第一个元素
//  @elem2: 需要比较的第二个元素
//  return: elem1 > elem2 大于0
//          elem2 = elem2 等于0
//          elem1 < elem2 小于0
//----------------------------------------------------------------------------
typedef int (*element_comparer)(void *elem1, void *elem2);


//----------------------------------------------------------------------------
//  比较函数: 比较函数需判断某个数组中的元素，是否满足检索的条件.
//            检索时会为每一个元素调用该函数
//  @arrayElem: 数组中的元素
//  @data:      检索的data
//  return:  满足检索条件返回0，否则返回非0
//----------------------------------------------------------------------------
typedef int (*element_matcher)(void *element, void *data);

//----------------------------------------------------------------------------
//  通用处理函数函数: 用于一般的线程调用，线程池调用等等
//            
// @param:     需要送入的参数
// @return:    处理完成的结果
//----------------------------------------------------------------------------
typedef void* (*imi_handler)(void* param, void* userdata);

//----------------------------------------------------------------------------
// 通用回调函数: 用于一般的回调场景
//            
// @data:      回调函数的结果
// @userdata:  设置回调时，用户设置的userdata。在回调时会带回，用于区分回调
// @返回值：   回调时可以不使用
//----------------------------------------------------------------------------
typedef void* (*imi_callback)(void* data, void* userdata);


//----------------------------------------------------------------------------
// hash函数定义: 用于一般的回调场景
//            
// @key:      需要hash的key
// @return:   hash后的值
//----------------------------------------------------------------------------
typedef unsigned (*imi_hash_func)(void *key);

#ifdef MEMWATCH

void* mwMalloc(size_t, const char*, int);
void* mwCalloc(size_t, size_t, const char*, int);
void* mwRealloc(void *, size_t, const char*, int);
char* mwStrdup(const char *, const char*, int);
void  mwFree(void*, const char*, int);


#define _IMI_NEW(struct_type, n_structs, file, line) \
        ((struct_type *) mwMalloc((n_structs) * sizeof (struct_type), file, line))

#define _IMI_MALLOC(size, file, line) mwMalloc(size, file, line)

#define _IMI_CALLOC(n, size, file, line) mwCalloc(n, size, file, line)

#define _IMI_REALLOC(ptr, size, file, line) mwRealloc(ptr, size, file, line)

#define _IMI_STRDUP(str, file, line) mwStrdup(str, file, line)

#define _IMI_FREE(ptr, file, line) mwFree(ptr, file, line)

#if defined (__GNUC__) && (__GNUC__ >= 2) && defined (__OPTIMIZE__)
#define _IMI_NEW0(struct_type, n_structs, file, line)   \
        ({                              \
            int size = (n_structs) * sizeof (struct_type);                          \
            void* ptr = mwMalloc(size, file, line); if(ptr) memset(ptr, 0, size);   \
            (struct_type*)ptr;                             \
        })
#else
void* __imi_mw_malloc0(size_t size, const char* file, int line);
#define _IMI_NEW0(struct_type, n_structs, file, line)   \
        ((struct_type *) __imi_mw_malloc0((n_structs) * sizeof (struct_type), file, line))
#endif

#else

#define _IMI_NEW(struct_type, n_structs, file, line)   \
        ((struct_type *) malloc((n_structs) * sizeof (struct_type)))

#define _IMI_MALLOC(size, file, line) malloc(size)

#define _IMI_CALLOC(n, size, file, line) calloc(n, size)

#define _IMI_REALLOC(ptr, size, file, line) realloc(ptr, size)

#define _IMI_STRDUP(str, file, line) clone_string(str)

#define _IMI_FREE(ptr, file, line) free(ptr)

#if defined  (__GNUC__) && (__GNUC__ >= 2) && defined (__OPTIMIZE__)
#define _IMI_NEW0(struct_type, n_structs, file, line)               \
        ({                                                          \
            int size = (n_structs) * sizeof (struct_type);          \
            void* ptr = malloc(size); if(ptr) memset(ptr, 0, size); \
            (struct_type*)ptr;                                      \
        })
#else

void* __imi_malloc0(size_t size);

#define _IMI_NEW0(struct_type, n_structs, file, line)   \
        ((struct_type *) __imi_malloc0((n_structs) * sizeof (struct_type)))
#endif

#endif


#define imi_new(struct_type, n_structs)	_IMI_NEW(struct_type, n_structs, __FILE__, __LINE__)

#define imi_new0(struct_type, n_structs) _IMI_NEW0(struct_type, n_structs, __FILE__, __LINE__)

#define imi_malloc(size) _IMI_MALLOC(size, __FILE__, __LINE__)

#define imi_calloc(n, size) _IMI_CALLOC(n, size, __FILE__, __LINE__)

#define imi_realloc(ptr, size) _IMI_REALLOC(ptr, size, __FILE__, __LINE__)

#define imi_strdup(str) _IMI_STRDUP(str, __FILE__,__LINE__)

#define imi_free(ptr) _IMI_FREE(ptr, __FILE__,__LINE__)

__END_DECLS

#endif // __DEFS_H__
