//////////////////////////////////////////////////////////
// 文件：FIFOQUEUE.h
//////////////////////////////////////////////////////////
#ifndef _FIFOQUEUE_H
#define _FIFOQUEUE_H

//#include "soc/rmt_struct.h"


#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif


typedef unsigned char uint8;
#define ElemType       uint8
#define QueueSize      2 //fifo队列的大小
#define QueueFull      0  //fifo满置0
#define QueueEmpty     1  //FIFO空置1
#define QueueOperateOk 2  //队列操作完成 赋值为2


struct FifoQueue
{
    unsigned short front;     //队列头
    unsigned short rear;        //队列尾
    unsigned short count;       //队列计数
//    ElemType dat[QueueSize];
	ElemType   dat[QueueSize];

};
//Queue Initalize
extern void QueueInit(struct FifoQueue *Queue);
// Queue In
extern uint8 QueueIn(struct FifoQueue *Queue,ElemType *sdat);
// Queue Out
extern uint8 QueueOut(struct FifoQueue *Queue,ElemType *sdat);
#endif

