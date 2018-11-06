#include <stdio.h>

#define VAL_TO_STR(str)  #str
#define  VERSION_CONCAT(x,y,z) (#x "." #y "." #z)
int main()
{
	printf("hello world version=%s\n", VERSION_CONCAT(1,2,3));
	printf("hello world version=%s\n", VAL_TO_STR(1));
	return (0);
}