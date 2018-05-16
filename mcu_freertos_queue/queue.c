//////////////////////////////////////////////////////////
// 文件：FIFOQUEUE.C
//////////////////////////////////////////////////////////
#include "queue.h"
#include "stdio.h"
typedef unsigned char  uint8;
typedef signed   char  int8;
typedef unsigned short uint16;
typedef signed   short int16;
//typedef unsigned int   uint32;
typedef signed   int   int32;
typedef float          fp32;

struct FifoQueue   q_test;

//Queue Init
void QueueInit(struct FifoQueue *Queue)
{
    Queue->front = Queue->rear;//初始化时队列头队列首相连
    Queue->count = 0;   //队列计数为0


	 printf("arch_os_mutex_create  ok %d\n",0 );
}

// Queue In
uint8 QueueIn(struct FifoQueue *Queue,ElemType *sdat) //数据进入队列
{
//	printf("111\n" );

	//int rslt = arch_os_mutex_get(Queue->q_lock, 10);
	//if (rslt != STATE_OK)
	//{
	//    printf("QueueIn arch_os_mutex_get  err %d\n", rslt );
	//	return -1;
	//}
//	printf("222\n" );
    if((Queue->front == Queue->rear) && (Queue->count >= QueueSize))
    {                    // full //判断如果队列满了
//	    arch_os_mutex_put(Queue->q_lock);
//		printf("333\n" );
        return QueueFull;    //返回队列满的标志
    }else
    {                    // in
//        Queue->dat[Queue->rear] = sdat;
		printf("444\n" );

      Queue->rear = (Queue->rear + 1) % QueueSize;
		 printf("666\n" );
      Queue->count = (Queue->count + 1);
		 printf("888\n" );
        return QueueOperateOk;
    }
}

// Queue Out
uint8 QueueOut(struct FifoQueue *Queue,ElemType *sdat)
{
	//int rslt = arch_os_mutex_get(Queue->q_lock, 10);
	//if (rslt != STATE_OK)
	//{
	//	printf("QueueOut arch_os_mutex_get  err %d\n", rslt );
	//	return -1;
	//}

    if((Queue->front == Queue->rear) && (Queue->count == 0))
    {                    // empty
 //  	 	arch_os_mutex_put(Queue->q_lock);
        return QueueEmpty;
    }
	else
    {                    // out
//		printf("QueueOut555\n" );


        Queue->front = (Queue->front + 1) % QueueSize;
        Queue->count = Queue->count - 1;
//		arch_os_mutex_put(Queue->q_lock);
//		printf("QueueOut666\n" );
        return QueueOperateOk;
    }
}

int main()
{
	uint8 test_data = 11;
	QueueInit(&q_test);
	QueueIn(&q_test , &test_data);
	printf("q_test.count1=%d\n", q_test.count);
	QueueIn(&q_test , &test_data);
	printf("q_test.count2=%d\n", q_test.count);
	QueueIn(&q_test , &test_data);
	printf("q_test.count3=%d\n", q_test.count);
	
	
	
	return 0;
	
}