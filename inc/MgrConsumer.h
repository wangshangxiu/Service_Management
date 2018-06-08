/*******************************************************************************************

*Department:	R&D Exchange 

*Decrible:  	ManagerSrv AssetsConsumer
				
				


*Auhor:			Savin

*Createdtime:	2018-03-13

*ModifyTime:	


********************************************************************************************/


#ifndef EXCHANGE_CONSUMER_HEAD_H
#define EXCHANGE_CONSUMER_HEAD_H

#include "tnode.h"
#include "xlogger.h"

#include <string>

using namespace std;
using namespace snetwork_xservice_tnode;


class CMgrConsumer : public TNodeConsumer
{
public:
	CMgrConsumer(const char *bindingkey, const char *queuename  )
	{
		
		m_bindingkey = bindingkey ;
		m_queuename = queuename;
	}
	~CMgrConsumer(){}
	
	string GetBindingkey(){ return m_bindingkey ; }
	string GetQueueName(){ return m_queuename ; }

	unsigned int ConsumerData( char *pMsg,int nMsgLen);
	

private:
	
	string	m_bindingkey ;
	string	m_queuename  ;

};




#endif //EXCHANGE_CONSUMER_HEAD_H

