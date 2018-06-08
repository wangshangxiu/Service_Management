
#include "Public.h"
#include "MgrConsumer.h"



unsigned int CMgrConsumer::ConsumerData( char *pMsg,int nMsgLen)
{
	if(nMsgLen >= CONSUMERBUFLEN)
	{
		XERROR("ConsumerData: Recv data is too long ,buflen:[%d],datalen:[%d],data[%s] \n ",CONSUMERBUFLEN,nMsgLen,pMsg);
		return 0;
	}
	
	STConsumerData *pNode = (STConsumerData *)g_pMemNodePool->GetEmptyNode();
	if(NULL == pNode )
	{
		XERROR("ConsumerData: Get DataNode from mempool fail,Recv data:[%d][%s] \n ",nMsgLen,pMsg);
		return 0;
	}


	const char *pTag = "\"Tag\":";
	const char *p = strstr(pMsg,pTag);
    if(NULL == p)
    {
        XINFO("ConsumerData:there is no 'Tag' field in json:[%d][%s] \n ",nMsgLen,pMsg);
        return 0;
    }

	pNode->nTag = atoi(p+strlen(pTag));

	XINFO("ConsumerData: Data:tag[0x%x],len[%d],msg[%s] \n ",pNode->nTag,nMsgLen,pMsg);
	
	memcpy(pNode->sConsumerBuf,pMsg,nMsgLen);
	
	switch(pNode->nTag)
	{
		case TAG_FREEZE:  					// 冻结请求: 订单挂单请求,出金请求
		case TAG_UNFREEZE:					// 解冻结请求: 取消订单,出金请求审核失败
		case TAG_DEPOSIT_POOL2USER:			// 入金通知 
		case TAG_DEPOSIT_USER2EXCHANGE:
		case TAG_DEPOSIT_EXCHANGE2POOL:
		case TAG_TRADESUCESS:				// 交易成功	
		case TAG_USERWITHDRAW:				// 出金请求
			g_pAssetsDataPool->PushDataNode(pNode);
			break;
		case TAG_LOGIN:
		case TAG_LOGOUT:
			g_pLoginoutDataPool->PushDataNode(pNode);
			break;
		default:
			g_pMgrDataPool->PushDataNode(pNode);
		
	}
		
	return 0;
}



static TNode *pTnode = NULL;

int InitConsumer() 
{
	STTnodeConfig tnodeconfig ;
    tnodeconfig.mq_vhost= g_stConfig.mq_vhost_token ;
    tnodeconfig.mq_exchange_group = g_stConfig.mq_exchange_token ;
    tnodeconfig.mq_host = g_stConfig.mq_address_token ;
    tnodeconfig.mq_port= g_stConfig.mq_port_token  ;
    tnodeconfig.mq_user = g_stConfig.mq_user_token ;
    tnodeconfig.mq_passwd= g_stConfig.mq_password_token ;

	pTnode = new TNode(tnodeconfig);
	assert( pTnode != NULL );
	if(-1 == pTnode->Init() )
    {
        XERROR("tnode init fail , pls check ! \n");
        return -1;
    }

	
	
	 CMgrConsumer *pConsumer = new CMgrConsumer( g_stConfig.mq_bindingkey_token.c_str() , g_stConfig.mq_queuename.c_str());
	 if(-1 == pTnode->AddTNodeConsumer(pConsumer) )
	{
		XERROR("AddTNodeConsumer  ERROR \n");
		pTnode->Close();
		return -1;
	}
	
	pTnode->RunConsumer();
		

	return 0;
}



