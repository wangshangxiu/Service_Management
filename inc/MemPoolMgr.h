/*******************************************************************************************

*Department:	Exchange

*Decrible:  	内存池和数据池的通用池子
				
				


*Auhor:			Savin Chen

*Createdtime:	2018-03-19

*ModifyTime:	


********************************************************************************************/


#ifndef DATAPOOL_HEAD_H
#define DATAPOOL_HEAD_H


#include <pthread.h>
#include <list>
#include <stdlib.h>
#include <string.h>

//#include "quickfix/STQuoteRequest44.h"



using namespace std;



#define NODELEN 1014


class CNodePool
{
public:
	CNodePool();
	int Init(int nNodeNumber,int nNodeSize);
	
	//内部会清零
	void PushUsedNode(void *arg);
	//内部不会清零
	void PushDataNode(void *arg);
	
	//获取业务数据,单线程调用
	void *GetDataNode();
	
	//获取空节点
	void *GetEmptyNode();
	
private:
	int 					m_nNodeNumber;
	int 					m_nNodeSize ;
	list<void *>			m_DataList;
	list<void *>			m_UnusedNodeList;
	pthread_mutex_t			m_DataLock;
	pthread_mutex_t			m_UnusedNodeLock;
	
};



#endif // DATAPOOL_HEAD_H




