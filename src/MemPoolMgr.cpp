
#include "MemPoolMgr.h"
#include "Public.h"




CNodePool::CNodePool()
{
	pthread_mutex_init(&m_DataLock,NULL);
	pthread_mutex_init(&m_UnusedNodeLock,NULL);
}

int CNodePool::Init(int nNodeNumber,int nNodeSize)
{
	m_nNodeNumber = nNodeNumber ;
	m_nNodeSize = nNodeSize ;
	
	for(int i=0;i<nNodeNumber;i++)
	{
	
		void *p = malloc(nNodeSize);
		if(NULL == p)return -1;
		memset(p,0, nNodeSize);
		m_UnusedNodeList.push_back(p);
	}
	return 0;
}


void CNodePool::PushDataNode(void *arg)
{
	assert(arg!=NULL);
	pthread_mutex_lock(&m_DataLock);
	m_DataList.push_back(arg);
	pthread_mutex_unlock(&m_DataLock);
	XINFO("PushDataNode:%p\n",arg);
}

void CNodePool::PushUsedNode(void *arg)
{
	assert(arg!=NULL);
	pthread_mutex_lock(&m_UnusedNodeLock);
	m_UnusedNodeList.push_back(arg);
	pthread_mutex_unlock(&m_UnusedNodeLock);
	XINFO("PushUsedNode:%p\n",arg);

}


void *CNodePool::GetEmptyNode()
{
	void *pNode = NULL; 
	pthread_mutex_lock(&m_UnusedNodeLock);
	{
		if(0 == m_UnusedNodeList.size())
		{
			pthread_mutex_unlock(&m_UnusedNodeLock);
			return NULL;
		}
		
		pNode = m_UnusedNodeList.front();
		m_UnusedNodeList.pop_front();
	}
	pthread_mutex_unlock(&m_UnusedNodeLock);
	
	memset(pNode,0,m_nNodeSize);
	XINFO("GetEmptyNode:%p\n",pNode);
	return pNode;;	
}



void *CNodePool::GetDataNode()
{
	void *pNode = NULL;
	if(2 < m_DataList.size())
	{
		pNode = m_DataList.front();
		m_DataList.pop_front();
		XINFO("GetDataNode:%p\n",pNode);
		return pNode;
	}
	
	pthread_mutex_lock(&m_DataLock);
	{
		if(0 == m_DataList.size())
		{
			pthread_mutex_unlock(&m_DataLock);
			return NULL;
		}
		
		pNode = m_DataList.front();
		m_DataList.pop_front();
	}
	pthread_mutex_unlock(&m_DataLock);
	XINFO("GetDataNode:%p\n",pNode);
	return pNode;
	
}


