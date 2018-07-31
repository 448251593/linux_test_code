#include<stdio.h>
#define RMT_TICK_10_US    (80000000/RMT_CLK_DIV/100000) 
#define RMT_CLK_DIV      100  
int main()
{
	int test;
	test = 800 / 10 * RMT_TICK_10_US;
	printf("test=%d\n", test);
	return 0;
}
