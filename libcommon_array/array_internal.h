/***********************************************************************************
* @ file    : array_internal.h
* @ author  : yang yang
* @ version : 0.9
* @ date    : 2016.08.15
* @ brief   : 动态数组的队列的内部实现. 仅供libcommon内部实现使用
* @ example : 
*             array_t arr = create_array(4, 8);
*             enqueue_array(arr, data1);
*             enqueue_array(arr, data2);
*             ...
* @Copyright (c) 2016  chuangmi inc.
************************************************************************************/

#ifndef __ARRAY_INTERNAL_H__
#define __ARRAY_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif
 
#include "array.h"

//-------------------------------------------------------------------------------------
// 读取动态数组的head位置 
// @return: 小于0: 错误编码
//-------------------------------------------------------------------------------------
int array_get_head_real_index(array_t arr);

//-------------------------------------------------------------------------------------
// 读取动态数组的tail位置
// @return: 小于0: 错误编码
//-------------------------------------------------------------------------------------
int array_get_tail_real_index(array_t arr);

//-------------------------------------------------------------------------------------
// 读取指定逻辑下标在动态数组中的实际下标
// @return: 小于0: 错误编码
//-------------------------------------------------------------------------------------
int array_get_real_index(array_t arr, int index);

// @brief:      根据当前的real index获取下一个有效元素的real index. 通常用于内部遍历
// @attention： 如果array发生了enlarge或者shrink操作后，使用老的index值会得到非预期结果
//              因此该接口通常用于固定大小的循环队列
// @return:     大于0表示下一个有效元素的下标。 小于0表示没有下一个有效元素
int array_get_next_real_index(array_t arr, int real_index);

// @brief:      根据实际下标值读取获取array的值. 通常用于内部遍历
// @attention： 如果array发生了enlarge或者shrink操作后，使用老的index值会得到非预期结果
//              因此该接口通常用于固定大小的循环队列
// @return:     如果指定real_index为无效值，则返回空
void* array_get_value_by_real_index(array_t arr, int real_index);

#ifdef __cplusplus
}
#endif

#endif // __ARRAY_INTERNAL_H__