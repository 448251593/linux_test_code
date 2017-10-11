#include<stdio.h>
#include <stdlib.h>
#include <sys/time.h>


void test_malloc()
{

	char  *pbuf;
	int i;
	struct timeval t_start,t_end;
	gettimeofday(&t_start, NULL);
	long start = t_start.tv_sec*1000+t_start.tv_usec/1000;
	//printf("start:%d\n", start);

	for (i = 1; i <= 10000; i++)
	{
		int size ;
		size =  1024*10;
		pbuf = (char *)malloc( size);
		if (pbuf != NULL)
		{
			memset(pbuf, i + 1, size);
			free(pbuf);
		}		
	}

	gettimeofday(&t_end, NULL);
	long endt = t_end.tv_sec*1000+t_end.tv_usec/1000;
	//printf("end:%d\n", t_end.tv_usec);
	printf("test_malloc %ld ms\n\n",endt - start);

	
}
void test_arr()
{

	char  pbuf[1024*10];
	int i;
	

	struct timeval t_start,t_end;
	gettimeofday(&t_start, NULL);
	long start = t_start.tv_sec*1000+t_start.tv_usec/1000;
	
	int	size =  1024*10;
	for (i = 1; i <= 10000; i++)
	{
		memset(pbuf, i + 1, size);	
	}

	gettimeofday(&t_end, NULL);
		long endt = t_end.tv_sec*1000+t_end.tv_usec/1000;
	//printf("end:%d\n", t_end.tv_usec);
	printf("test_arr %ld ms\n\n",endt - start);


}

int main(int argc, char* argv[])
{

	

	/*int size = sizeof(MOTION_DETECTION_REPORT)+3 * sizeof(MOTION_DETECTION_REPORT_UNIT);
	MOTION_DETECTION_REPORT *report = (MOTION_DETECTION_REPORT *)malloc(size);

	memset(report, 0x0, size);
	report->unit[0].key[0] = '3';
	report->unit[1].key[0] = '4';
	report->unit[2].key[0] = '5';
	printf("-->%d\n", sizeof(MOTION_DETECTION_REPORT));
	getchar();*/
	
	test_arr();
	test_malloc();
	//getchar();
	return 0;
}