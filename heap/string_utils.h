/**************************************************************************
 * @ file    :  string_utils.h
 * @ author  :  yang yang
 * @ version :  0.9
 * @ date    :  2016.08.15
 * @ brief   :
 * @Copyright (c) 2016  chuangmi inc.
***************************************************************************/

#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------
//  @brief   从指定buffer中copy第一个以delimeter结尾的字符串, 放到pToken中.
//           token的内存将在函数中分配, 调用者必须自己处理相关的内存释放
//  @param   buffer   需要处理的buffer
//  @param   outToken  输出的token
//  @param   deliemter
//
//  @return:  当前处理到的位置
//-----------------------------------------------------------------------
char *read_token(char* buffer,  char** outToken, char delimeter, bool mustEnd);


//-----------------------------------------------------------------------
//
//  @brief   split指定字符串. 输出token的内存将在函数中分配,  调用者必须自己处理相关的内存释放
//  @param   src   需要split的字符串
//  @param   delimeter   分隔符
//  @param   tokenArray  保存的token
//  @param   maxSize     tokenArray的最大长度
//  @return: 找到的token个数
 //-----------------------------------------------------------------------
int split(char *src, char delimeter, char** tokenArray, int maxSize);

//-----------------------------------------------------------------------
// @brief   检测字符串是否以指定串开始
//-----------------------------------------------------------------------
bool str_start_with(char *src, char* prefix);

////-----------------------------------------------------------------------
//// @brief   复制字符串. 内存在该函数中分配, 调用者需要负责回收
////-----------------------------------------------------------------------
//char *clone_string(char* value);
//
////-----------------------------------------------------------------------
//// @brief   释放clone_string分配处理的字符串
////-----------------------------------------------------------------------
//void release_string(char* value);

//-----------------------------------------------------------------------
// @brief   去除字符串两端的空白字符。将会在字符串原地修改
// @str     需要处理的字符串
// @return  处理完成的字符（也是原串）
//-----------------------------------------------------------------------
char* str_trim(char *str);
char* str_trim_start(char *str);
char* str_trim_end(char *str);

char *str_read_token(char* str, char* outBuffer, int bufferSize, char delimeter, bool mustEnd);

//-----------------------------------------------------------------------
// @brief 获取指定从第n个delimeter开始，2个delimiter之间的token，并复制到
//        指定的buffer中。token两端的空格会被跳过
//        如果起始delimiter之后再也没有delimiter，则截取到字结束为止。
//        如果指定buffer不够大，则到填满buffer为止.
//
// @return 第二个delimeter所在位置或是字符串结束位置. NULL表明copy失败
//-----------------------------------------------------------------------
char *str_read_token_between(char* str, char start, char end,
    char* outBuffer, int maxBufferSize, int skipN);

//-----------------------------------------------------------------------
// @brief 计算string的hash值
//-----------------------------------------------------------------------
unsigned string_hash(const char * key);

//-----------------------------------------------------------------------
// @brief   复制字符串. 内存在该函数中分配, 调用者需要负责回收
//-----------------------------------------------------------------------
char *clone_string(char* value);

//-----------------------------------------------------------------------
// @brief   释放clone_string分配处理的字符串
//-----------------------------------------------------------------------
void release_string(char* value);


// 从文件中读取所有文本至缓冲区当中
int imi_read_all_text(const char *path, char* buffer, int bufferSize);

// 从文件中读取所有文本
// 调用者需要自己释放内存
char* imi_read_all_text_new(const char *path, int* out_size);



#ifdef __cplusplus
}
#endif

#endif // __STRING_UTILS_H__
