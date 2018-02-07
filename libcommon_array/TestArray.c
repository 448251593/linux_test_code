/**************************************************************************
* @ file:       TestArray.c
* @ author :    yang yang
* @ version :   0.9
* @ date :      2016.08.15
* @ brief :     ��̬����UnitTest
* @Copyright(c) 2016  chuangmi inc.
***************************************************************************/

#include "unity.h"
#include "unity_fixture.h"
#include "array.h"
#include "array_internal.h"


// ---------------------------------------------------------------------------
// �����������(Group, TestName)
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
	
	
    // ���Ի�����ջ����
    //TEST_ASSERT_NOT_EQUAL(0, array_push(arr, data)); // ��ʱӦ���޷���ջ, Ԫ��Ϊ2,4,5,6, head=2, tail=1
    //TEST_ASSERT_EQUAL(6, *((int*)array_pop(arr))); // ��ʱӦ�ð�6����
    //TEST_ASSERT_EQUAL(2, array_get_head_real_index(arr)); // ��ʱhead = 2, tail = 0
    //TEST_ASSERT_EQUAL(0, array_get_tail_real_index(arr));

    release_array(arr);
	return 0;
}
