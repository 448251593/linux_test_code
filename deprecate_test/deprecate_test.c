#include<stdio.h>
 #define THIS_FUNCTION_IS_DEPRECATED(func) func __attribute__ ((deprecated))

void test()
{

	printf("test deprecate function\n");
}

THIS_FUNCTION_IS_DEPRECATED (extern void test());

int main()
{

	test();
}

