//////////////////////////////////////////////////////////
// �ļ���FIFOQUEUE.h
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
#define QueueSize      2 //fifo���еĴ�С
#define QueueFull      0  //fifo����0
#define QueueEmpty     1  //FIFO����1
#define QueueOperateOk 2  //���в������ ��ֵΪ2


struct FifoQueue
{
    unsigned short front;     //����ͷ
    unsigned short rear;        //����β
    unsigned short count;       //���м���
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

