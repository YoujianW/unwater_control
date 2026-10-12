#include "circle_queue.h"

#define QUEUE_MAX_SIZE 255
#define FULL_RESET 0
#define FULL_DROP_OLD 1
#define FULL_BEHAVIOUR FULL_DROP_OLD

typedef struct _QUEUE                                             
{                                                                 
    uint16_t _head;                 //队列头
    uint16_t _tail;                 //队列尾
    uint8_t _data[QUEUE_MAX_SIZE];  //队列数据缓存区
}QUEUE;                                                           
static QUEUE que = {0,0,0};      //队列

//获取队列中有效数据个数
static uint32_t queue_size(void)
{
    return ((que._head+QUEUE_MAX_SIZE-que._tail)%QUEUE_MAX_SIZE);
}

//获取队列中有效数据个数
uint16_t Que_GetRxSize(void)
{
	return(queue_size());
}

//清除队列
void queue_reset(void)
{
    que._head = que._tail = 0;
}

//添加一个数据到队列
uint8_t queue_push(uint8_t _data) 
{
    uint16_t pos = (que._head+1)%QUEUE_MAX_SIZE;
    if(pos!=que._tail)                 //非满状态
    {
        que._data[que._head] = _data;
        que._head = pos;
		return(1);
    }
    else
    {        
        if(FULL_BEHAVIOUR == FULL_RESET)
        {
            queue_reset();
            pos = 1;
            que._data[que._head] = _data;
            que._head = pos;
            return 0;
        }
        if(FULL_BEHAVIOUR == FULL_DROP_OLD)
        {
            que._data[que._head] = _data;  
            que._head = pos;               
            que._tail = (que._tail+1)%QUEUE_MAX_SIZE;   
            return(1);                
        }
    }
}

static uint8_t queue_pop(uint8_t* _data)
{
    if(que._tail!=que._head)            //非空状态
    {
        *_data = que._data[que._tail];
        que._tail = (que._tail+1)%QUEUE_MAX_SIZE;
        return 1;
    }
    else
        return 0;
}

//从队列中取一个数据
uint8_t Que_ReadByte(uint8_t* pIByte)
{	
	if(queue_size()>0)
	{
		//取一个数据
        queue_pop(pIByte);
        return 1;
	}
    else
	{
        return 0;
	}
}

//从队列中取多个数据
uint16_t Que_ReadMultiBytes(uint8_t* pIByte, uint16_t len)
{
    uint16_t i = 0;
    
    if (len <= queue_size()) 
    {
        for (i = 0; i < len; i++) {
            Que_ReadByte(&pIByte[i]);
        }
        return len;
    } 
    else 
    {
        return 0;
    }
}

