/**************************************************************************
 * @ file    :   array.c
 * @ author  : yang yang
 * @ version : 0.9
 * @ date    :   2016.08.15
 * @ brief   :
 * @Copyright (c) 2016  chuangmi inc.
***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "array.h"
#include "array_internal.h"

typedef struct _array_t {
    int size;
    int capacity;
    int min_capacity;
    int max_capacity;
    int head;
    int tail;
    void **data;
} _array_t;

#pragma region inner methods

// 扩大动态数组容量为原来的2倍
// @return: 0 成功，其他错误编码。请参考头文件中宏定义
static int enlarge_array(array_t arr) {
    if (arr == NULL) {
        return CODE_INVALID_PARAM;
    }

    int newCapacity = arr->capacity * 2;
    if (newCapacity > arr->max_capacity) {
        return CODE_MAX_CAPACITY_REACH;
    }

    size_t newSize = sizeof(void *) * newCapacity;
    void **data = imi_malloc(newSize);
    memset(data, 0, newSize);
    if (arr->head <= arr->tail) {
        memcpy(data, arr->data + arr->head, sizeof(void *) * arr->size);
    }
    else {
        // copy [head, array_end] to new array [0, size]
        int size = sizeof(void *) * (arr->capacity - arr->head);
        memcpy(data, arr->data + arr->head, size);
        size = sizeof(void *) * (arr->tail + 1);
        memcpy(data + arr->capacity - arr->head, arr->data, size);
    }

    imi_free(arr->data);

    arr->head = 0;
    arr->tail = arr->size - 1;

    arr->data = data;
    arr->capacity = newCapacity;
    return CODE_SUCCESS;
}

// 压缩动态数组为原来的1/2
// @return: 0 成功，其他错误编码。请参考头文件中宏定义
static int shrink_array(array_t arr) {
    if (arr == NULL) {
        return CODE_INVALID_PARAM;
    }

    int newCapacity = arr->capacity / 2;
    if (arr->size > newCapacity || newCapacity < arr->min_capacity) {
        return QUEUE_ERR_CANNOT_SHRINK;
    }

    size_t newSize = sizeof(void *) * newCapacity;
    void **data = imi_malloc(newSize);
    memset(data, 0, newSize);
    if (arr->head <= arr->tail) {
        memcpy(data, arr->data + arr->head, sizeof(void *) * arr->size);
    }
    else {
        // copy [head, array_end] to new array [0, size]
        int size = sizeof(void *) * (arr->capacity - arr->head);
        memcpy(data, arr->data + arr->head, size);
        size = sizeof(void *) * (arr->tail + 1);
        memcpy(arr + arr->capacity - arr->head, arr->data, size);
    }

    imi_free(arr->data);

    arr->head = 0;
    arr->tail = arr->size - 1;

    arr->data = data;
    arr->capacity = newCapacity;
    return CODE_SUCCESS;
}

static int array_get_real_index_inner(array_t arr, int index) {
    return (index + arr->head) % arr->capacity;
}

#pragma endregion

#pragma region interface methods

array_t create_array(int initCapacity, int maxCapacity) {
    array_t arr = imi_malloc(sizeof(_array_t));
    arr->size = 0;
    arr->capacity = initCapacity;
    arr->min_capacity = initCapacity;
    arr->max_capacity = maxCapacity;
    arr->head = arr->tail = 0;

    size_t len = sizeof(void *) * initCapacity;
    arr->data = imi_malloc(len);
    memset(arr->data, 0, len);
    return arr;
}

void release_array(array_t arr) {
    if (arr != NULL && arr->data != NULL) {
        imi_free(arr->data);
        imi_free(arr);
    }
}

int array_get_size(array_t arr) {
    return arr == NULL ? CODE_INVALID_PARAM : arr->size;
}

int array_get_capacity(array_t arr) {
    return arr == NULL ? CODE_INVALID_PARAM : arr->capacity;
}

int array_get_max_capacity(array_t arr) {
    return arr == NULL ? CODE_INVALID_PARAM : arr->max_capacity;
}

int array_get_min_capacity(array_t arr) {
    return arr == NULL ? CODE_INVALID_PARAM : arr->min_capacity;
}

void *array_get_value(array_t arr, int index) {
    int realIdx = array_get_real_index(arr, index);
    return realIdx < 0 ? NULL : arr->data[realIdx];
}

int array_update_value(array_t arr, int index, void* data) {
    int idx = array_get_real_index(arr, index);
    if (idx >= 0) {
        arr->data[idx] = data;
        return CODE_SUCCESS;
    }
    else {
        return idx;
    }
}

int array_search(array_t arr, int startIdx, element_matcher matcher, void *searchData) {
    if (arr == NULL || matcher == NULL || startIdx < 0 || startIdx >= arr->size) {
        return CODE_INVALID_PARAM;
    }

    int i;
    for (i = startIdx; i < arr->size; i++) {
        int index = array_get_real_index_inner(arr, i);
        if ((*matcher)(arr->data[index], searchData) == 0) {
            return i;
        }
    }

    return CODE_NOT_FOUND;
}

void *array_remove(array_t arr, int index) {
    int realIdx = array_get_real_index(arr, index);
    if (realIdx < 0) {
        return NULL;
    }

    void *result = arr->data[realIdx];

    // 移动数据
    int i;
    for (i = index; i < arr->size - 1; i++) {
        arr->data[array_get_real_index_inner(arr, i)] = arr->data[array_get_real_index_inner(arr, i+1)];
    }

    arr->size--;
    arr->tail--;
    if (arr->tail < 0) {
        arr->tail = arr->capacity - 1;
    }

    return result;
}

void *array_search_and_remove(array_t arr, int startIdx, element_matcher comparer, void *searchData) {
    int index = array_search(arr, startIdx, comparer, searchData);
    return index >= 0 ? array_remove(arr, index) : NULL;
}

void *array_fast_remove(array_t arr, int index) {
    int realIdx = array_get_real_index(arr, index);
    if (realIdx < 0) {
        return NULL;
    }

    void *result = arr->data[realIdx];
    arr->data[realIdx] = arr->data[arr->tail];

    arr->size--;
    arr->tail--;
    if (arr->tail < 0) {
        arr->tail = arr->capacity - 1;
    }

    return result;
}

void *array_fast_search_and_remove(array_t arr, int startIdx, element_matcher comparer, void *searchData) {
    int index = array_search(arr, startIdx, comparer, searchData);
    return index >= 0 ? array_fast_remove(arr, index) : NULL;
}

int array_enqueue(array_t arr, void *data) {
    if (arr != NULL) {
        if (arr->size == 0) {
            arr->head = arr->tail = 0;
            arr->size = 1;
            arr->data[0] = data;
            return CODE_SUCCESS;
        } else {
            if (arr->size == arr->capacity) {
                int ret = enlarge_array(arr);
                if (ret != CODE_SUCCESS) {
                    return ret;
                }
            }

            int index = (arr->tail + 1) % arr->capacity;
            arr->data[index] = data;
            arr->tail = index;
            arr->size++;

            return CODE_SUCCESS;
        }
    } else {
        return CODE_INVALID_PARAM;
    }
}

void *array_dequeue(array_t arr) {
    if (arr != NULL && arr->size > 0) {
        void *result = arr->data[arr->head];
        arr->head = (arr->head + 1) % arr->capacity;
        arr->size--;

        return result;
    }

    return NULL;
}

void *array_peek_queue(array_t arr) {
    if (arr != NULL && arr->size > 0) {
        return arr->data[arr->head];
    }

    return NULL;
}

int array_push(array_t arr, void *data) { 
  int ret = array_enqueue(arr, data);// 插入元素的方式与enqueue一致, 从队尾插入
  return ret;
}

void *array_pop(array_t arr) {
    if (arr != NULL && arr->size > 0) {
        void *result = arr->data[arr->tail];
        arr->tail = arr->tail - 1;
        if (arr->tail < 0) {
            arr->tail = arr->capacity - 1;
        }

        arr->size--;
        return result;
    }

    return NULL;
}

void *array_peek_stack(array_t arr) {
    if (arr != NULL && arr->size > 0) {
        return arr->data[arr->tail];
    }

    return NULL;
}

#pragma endregion

#pragma region internal interface methods

int array_get_head_real_index(array_t arr) {
    return arr == NULL ? CODE_INVALID_PARAM : arr->head;
}

int array_get_tail_real_index(array_t arr) {
    return arr == NULL ? CODE_INVALID_PARAM : arr->tail;
}

int array_get_real_index(array_t arr, int index) {
    return (arr == NULL || index < 0 || index >= arr->size) ? 
        CODE_INVALID_PARAM : array_get_real_index_inner(arr, index);
}

int array_get_next_real_index(array_t arr, int real_index) {
    return (arr->size <= 1 || real_index < arr->head || real_index >= arr->tail) ?
        CODE_INVALID_PARAM : array_get_real_index_inner(arr, real_index + 1);
}

void* array_get_value_by_real_index(array_t arr, int real_index) {
    return (arr->size == 0 || real_index < arr->head || real_index > arr->tail) ?
        NULL : arr->data[real_index];
}

#pragma endregion
