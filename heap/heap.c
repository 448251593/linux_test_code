/**************************************************************************
* @ file    : heap.c
* @ author  : yang yang
* @ version : 0.9
* @ date    : 2017.08.01
* @ brief   :
* @Copyright (c) 2016  chuangmi inc.
***************************************************************************/

#include <stdlib.h>

#include "defs.h"
#include "heap.h"

typedef struct _heap_t {
    int  size;

    int  capacity;
    int  max_capacity;

    element_comparer comparer;
    void    **data;
} _heap_t;

//---------------------------------------------------------------------------
//  内部函数
//---------------------------------------------------------------------------

//--------------------------------------------------------------
// 获取父结点位置 (i-1)/2
// @param npos 当前结点位置
// @return 父结点位置
//--------------------------------------------------------------
#define heap_parent(npos) ((int)(((npos) - 1) / 2))

//--------------------------------------------------------------
// 获取左子结点位置 i*2+1
// @param npos 当前结点位置
// @return 左子结点位置
//--------------------------------------------------------------
#define heap_left(npos) (((npos) * 2) + 1)

//--------------------------------------------------------------
// 获取右子结点位置 i*2+2
// @param npos 当前结点位置
// @return 右子结点位置
//--------------------------------------------------------------
#define heap_right(npos) (((npos) * 2) + 2)


static int ensure_capacity(heap_t heap) {
    if (heap->size < heap->capacity)
        return CODE_SUCCESS;

    int size = heap->size * 2;
    if (size > heap->max_capacity) {
        return CODE_MAX_CAPACITY_REACH;
    }

    void** data = (void **)imi_realloc(heap->data, size * sizeof(void*));
    if (data == NULL) {
        return CODE_MALLOC_FAILED;
    }

    heap->capacity = size;
    heap->data = data;
    return CODE_SUCCESS;
}

// 从指定节点开始下沉堆化操作. 用于删除流程
static void heapify_rolldown(heap_t heap, int index) {
    // 重新heapify
    int current = index;
    int selected;
    while (1) {
        // 选择与当前节点交换的节点
        int left = heap_left(current);
        int right = heap_right(current);

        selected = (left < heap->size && heap->comparer(heap->data[left], heap->data[current]) > 0) ? left : current;
        selected = (right < heap->size && heap->comparer(heap->data[right], heap->data[selected]) > 0) ? right : selected;
        if (selected == current) {
            break; // 如果没有产生任何交换，已经调整完成
        }
        else {
            // swap节点
            void* temp = heap->data[selected];
            heap->data[selected] = heap->data[current];
            heap->data[current] = temp;

            current = selected;
        }
    }
}

//---------------------------------------------------------------------------
//  接口函数
//---------------------------------------------------------------------------
heap_t heap_create(element_comparer comparer, int initSize, int maxSize) {
    heap_t heap = imi_malloc(sizeof(_heap_t));
    if (heap != NULL) {
        heap->size = 0;
        heap->comparer = comparer;

        heap->capacity = initSize;
        heap->max_capacity = maxSize;

        heap->data = imi_malloc(sizeof(void*) * initSize);
    }

    return heap;
}

void heap_release(heap_t heap) {
    if (heap != NULL) {
        if (heap->data) {
            imi_free(heap->data);
        }

        imi_free(heap);
    }
}

int heap_insert(heap_t heap, void *data) {
    if (heap == NULL || data == NULL) {
        return CODE_INVALID_PARAM;
    }

    int ret = ensure_capacity(heap);
    if (ret == CODE_SUCCESS) {
        // 将结点插入到最后位置
        heap->data[heap->size] = data;

        // heapify
        int current = heap->size;
        int parent = heap_parent(current);
        while (current > 0 && heap->comparer(heap->data[parent], heap->data[current]) < 0) {
            // swap节点
            void *temp = heap->data[parent];
            heap->data[parent] = heap->data[current];
            heap->data[current] = temp;

            current = parent;
            parent = heap_parent(current);
        }

        // 更新堆的 size
        heap->size++;
    }

    return ret;
}

void* heap_pop(heap_t heap) {
    if (heap->size <= 0) {
        return NULL;
    }

    void* top = heap->data[0];
    heap->size--;
    if (heap->size > 0) {
        // 将最后一个元素移到堆顶，从堆顶开始重新堆化
        heap->data[0] = heap->data[heap->size];
        heapify_rolldown(heap, 0);
    }

    return top;
}

void* heap_peek(heap_t heap) {
    return heap->size <= 0 ? NULL : heap->data[0];
}

int get_heap_size(heap_t heap) {
    return heap != NULL ? heap->size : CODE_INVALID_PARAM;
}

int get_heap_capacity(heap_t heap) {
    return heap != NULL ? heap->capacity : CODE_INVALID_PARAM;
}

int get_heap_max_capacity(heap_t heap) {
    return heap != NULL ? heap->max_capacity : CODE_INVALID_PARAM;
}

int heap_search(heap_t heap, int startIdx, element_matcher matcher, void *searchData) {
    if (heap == NULL || matcher == NULL || startIdx < 0 || startIdx >= heap->size) {
        return CODE_INVALID_PARAM;
    }

    int i;
    for (i = startIdx; i < heap->size; i++) {
        if ((*matcher)(heap->data[i], searchData) == 0) {
            return i;
        }
    }

    return -1;
}

void *heap_remove(heap_t heap, int index) {
    if (heap == NULL || index < 0 || index >= heap->size) {
        return NULL;
    }

    void* removed = heap->data[index];
    heap->size--;

    if (heap->size > 0) {
        // 将最后一个元素移到堆顶，从堆顶开始重新堆化
        heap->data[index] = heap->data[heap->size];
        heapify_rolldown(heap, index);
    }

    return removed;
}