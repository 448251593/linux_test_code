#include<stdio.h>
#include<heap.h>

int cmp(void *e1,void *e2)
{
	//return  *((int *)e1) < *((int *)e2) ? 1 : 0;
		int a = *((int *)e1);
		int b = *((int *)e2);
		
	if(a < b)
	{
		printf("e1<e2 ,%d,%d\n",a,b);
		return 1;
	}
	else
	{
		printf("e1>=e2 ,%d,%d\n",a,b);
		return -1;
	}
}
 
int main()
{
	
	heap_t  h;
	h = heap_create(cmp, 10, 10);
	
	int v = 1;
	heap_insert(h,(void*)&v);
	int v1 = 11;
	heap_insert(h,(void*)&v1);
	int v2 = 12;
	heap_insert(h,(void*)&v2);
	int v3 = 15;
	heap_insert(h,(void*)&v3);
	int v4 = 17;
	heap_insert(h,(void*)&v4);
	
	printf("top=%d\n", *((int*)heap_peek(h)));
	
	
	
}
