#include<stdio.h>
#include<stdlib.h>
int main()
{

	char *macstr="78:11:DC:39:56:62";
	char arr[6];

	sscanf(macstr, "%x:%x:%x:%x:%x:%x",&arr[0],&arr[1],&arr[2],&arr[3],&arr[4],&arr[5]);
	int i;
	for(i = 0 ; i< 6; i++)
	{
		printf("%x,",arr[i]);
	}
	return 0;
}
