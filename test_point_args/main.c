#include<stdio.h>


char *buf = "12345";
int  len = 0;

int  get_p(char **p, int *len)
{
	*p = buf;
	*len = 12;
	
	
}

int main()
{
	
	
	char *pt;
	int len;
	get_p(&pt, &len);
	printf("%s, len=%d\n", pt, len);
	
	return 0;
}