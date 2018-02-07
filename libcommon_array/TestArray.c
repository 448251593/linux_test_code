/**************************************************************************
* @ file:       TestArray.c
* @ author :    yang yang
* @ version :   0.9
* @ date :      2016.08.15
* @ brief :     动态数组UnitTest
* @Copyright(c) 2016  chuangmi inc.
***************************************************************************/

#include "unity.h"
#include "unity_fixture.h"
#include "array.h"
#include "array_internal.h"


// ---------------------------------------------------------------------------
// 定义测试用例(Group, TestName)
// ---------------------------------------------------------------------------

int main()
{
   // int data[7] = { 0, 1, 2, 3, 4, 5, 6 };

    array_t arr = create_array(10, 10);

	int data;
	data = 1;
	array_push(arr, (void*)&data);
	int data1 = 2;
	array_push(arr, (void*)&data1);	
	int  data3 = 3;
	array_push(arr, (void*)&data3);
	int  data4 = 4;
	array_push(arr, (void*)&data4);	
	int  data5 = 5;
	array_push(arr, (void*)&data5);	
	int  data6 = 6;
	array_push(arr, (void*)&data6);
	
	//printf("1\n");

	
	printf("%d\n",*((int*)array_pop(arr)));
	//array_remove(arr,array_get_tail_real_index(arr));
	printf("%d\n",*((int*)array_pop(arr)));
	//array_remove(arr,array_get_tail_real_index(arr));
	printf("%d\n",*((int*)array_pop(arr)));
	//array_remove(arr,array_get_tail_real_index(arr));
	printf("%d\n",*((int*)array_pop(arr)));
	//array_remove(arr,array_get_tail_real_index(arr));
	printf("%d\n",*((int*)array_pop(arr)));
	//array_remove(arr,array_get_tail_real_index(arr));
	printf("%d\n",*((int*)array_pop(arr)));
	//array_remove(arr,array_get_tail_real_index(arr));
	
	
    // 测试基本入栈操作
    //TEST_ASSERT_NOT_EQUAL(0, array_push(arr, data)); // 此时应该无法入栈, 元素为2,4,5,6, head=2, tail=1
    //TEST_ASSERT_EQUAL(6, *((int*)array_pop(arr))); // 此时应该把6弹出
    //TEST_ASSERT_EQUAL(2, array_get_head_real_index(arr)); // 此时head = 2, tail = 0
    //TEST_ASSERT_EQUAL(0, array_get_tail_real_index(arr));

    release_array(arr);
	return 0;
}
