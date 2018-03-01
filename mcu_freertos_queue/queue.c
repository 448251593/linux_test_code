//////////////////////////////////////////////////////////
// 文件：FIFOQUEUE.C
//////////////////////////////////////////////////////////
#include "queue.h"

typedef unsigned char  uint8;
typedef signed   char  int8;
typedef unsigned short uint16;
typedef signed   short int16;
//typedef unsigned int   uint32;
typedef signed   int   int32;
typedef float          fp32;



//Queue Init
void QueueInit(struct FifoQueue *Queue)
{
    Queue->front = Queue->rear;//初始化时队列头队列首相连
    Queue->count = 0;   //队列计数为0

	int rslt = arch_os_mutex_create(&Queue->q_lock);
	if (rslt != STATE_OK)
	{
	    ets_printf("arch_os_mutex_create  err %d\n", rslt );
	}
	 ets_printf("arch_os_mutex_create  ok %d\n", rslt );
}

// Queue In
uint8 QueueIn(struct FifoQueue *Queue,ElemType *sdat) //数据进入队列
{
//	ets_printf("111\n" );

	int rslt = arch_os_mutex_get(Queue->q_lock, 10);
	if (rslt != STATE_OK)
	{
	    ets_printf("QueueIn arch_os_mutex_get  err %d\n", rslt );
		return -1;
	}
//	ets_printf("222\n" );
    if((Queue->front == Queue->rear) && (Queue->count == QueueSize))
    {                    // full //判断如果队列满了
	    arch_os_mutex_put(Queue->q_lock);
//		ets_printf("333\n" );
        return QueueFull;    //返回队列满的标志
    }else
    {                    // in
//        Queue->dat[Queue->rear] = sdat;
//		ets_printf("444\n" );

		Queue->dat[Queue->rear].wave_data_len = sdat->wave_data_len;
		int i = 0;
		for ( i = 0; i < sdat->wave_data_len; i++ )
		{
		    Queue->dat[Queue->rear].infrared_wave_data[i] = sdat->infrared_wave_data[i];
		}
//		 ets_printf("555\n" );
        Queue->rear = (Queue->rear + 1) % QueueSize;
//		 ets_printf("666\n" );
        Queue->count = Queue->count + 1;
//		 ets_printf("777\n" );
		arch_os_mutex_put(Queue->q_lock);
//		 ets_printf("888\n" );
        return QueueOperateOk;
    }
}

// Queue Out
uint8 QueueOut(struct FifoQueue *Queue,ElemType *sdat)
{
	int rslt = arch_os_mutex_get(Queue->q_lock, 10);
	if (rslt != STATE_OK)
	{
		printf("QueueOut arch_os_mutex_get  err %d\n", rslt );
		return -1;
	}

    if((Queue->front == Queue->rear) && (Queue->count == 0))
    {                    // empty
   	 	arch_os_mutex_put(Queue->q_lock);
        return QueueEmpty;
    }
	else
    {                    // out
//		ets_printf("QueueOut555\n" );

		sdat->wave_data_len = Queue->dat[Queue->front].wave_data_len;

		int i = 0;
		for ( i = 0; i < Queue->dat[Queue->front].wave_data_len; i++ )
		{
			sdat->infrared_wave_data[i] = Queue->dat[Queue->front].infrared_wave_data[i];
		}

        Queue->front = (Queue->front + 1) % QueueSize;
        Queue->count = Queue->count - 1;
		arch_os_mutex_put(Queue->q_lock);
//		ets_printf("QueueOut666\n" );
        return QueueOperateOk;
    }
}

