
#include "MgrBase.h"


CMgrBase::CMgrBase()
{
	m_pTnode = NULL;
	m_pRedis = NULL;
	m_pDb = NULL;
	
	m_nFail = 0; // -1 DB操作失败
}

CMgrBase::~CMgrBase()
{
	if(NULL != m_pTnode)
	{
		m_pTnode->Close();
		delete m_pTnode;
		m_pTnode = NULL;
	}
	
	if(NULL != m_pRedis)
	{
		delete m_pRedis;
		m_pRedis = NULL;
	}
	
	if(NULL != m_pDb)
	{
		m_pDb->Close();
		delete m_pDb;
		m_pDb = NULL;
	}
}


int CMgrBase::CheckCode(const char *pKey, const char *pCode, const char *pJson)
{
	//for test
	return 0;

	if( !m_pRedis->IsConnect() )
    {
        XERROR("redis init fail , pls check ! \n");
        return -1;
    }

	//查询redis
	string sCode = m_pRedis->HGet(pKey,"code");
	string sTime = m_pRedis->HGet(pKey,"time");

	XINFO("CheckCode:key:%s,code1:%s ,code2:%s,time:%s \n",pKey,pCode,sCode.c_str(),sTime.c_str() );
	
	time_t nowtime = time(NULL);
	if(nowtime > (atol(sTime.c_str()) + SMSCODETIMEOUT))
	{
		// 超时
		XERROR("CheckCode: timeout,json:[%s]\n",pJson);
		//m_pRedis->Del(pKey);
		return -1;
	}
		
	if(0 != sCode.compare(pCode))
	{
		// sms验证码不一致
		XERROR("CheckCode: checkcode[%s] is not same,json:[%s]\n",sCode.c_str(),pJson);
		//m_pRedis->Del(pKey);
		return -1;
	}
		
	//m_pRedis->Del(pKey);
	
	return 0;
}




int CMgrBase::Init()
{
	STTnodeConfig tnodeconfig ;
    tnodeconfig.mq_vhost= g_stConfig.mq_vhost_token ;
    tnodeconfig.mq_exchange_group = g_stConfig.mq_exchange_token ;
    tnodeconfig.mq_host = g_stConfig.mq_address_token ;
    tnodeconfig.mq_port= g_stConfig.mq_port_token  ;
    tnodeconfig.mq_user = g_stConfig.mq_user_token ;
    tnodeconfig.mq_passwd= g_stConfig.mq_password_token ;

	m_pTnode = new TNode(tnodeconfig);
	assert( m_pTnode != NULL );
	if(-1 == m_pTnode->Init() )
    {
        XERROR("tnode init fail , pls check ! \n");
        return -1;
    }

	
	m_pRedis = new CRedis(g_stConfig.sRedisHost.c_str(), g_stConfig.nRedisPort, g_stConfig.nRedisDBIndex , g_stConfig.sRedisPwd.c_str());
    assert( m_pRedis != NULL );
	if( !m_pRedis->IsConnect() )
    {
        XERROR("redis init fail , pls check ! \n");
        return -1;
    }

	
	m_pDb = new MySqlDB();
    m_pDb->Open();
    m_pDb->SelectDB(g_stConfig.sDBName.c_str());

	return 0;
}




int CMgrBase::GetDataListFromDB(MySqlRecordSet &rs,const char *pSQL, const char *pModuleName)
{

	rs = m_pDb->QuerySql(pSQL);
	if (m_pDb->IsError()) 
	{
		XERROR("%s: QuerySql:%s,con=%d|sql=%s",pModuleName ,m_pDb->GetMySqlErrMsg(), m_pDb->IsConnect(), pSQL);
		m_nFail = -1;
		return -1;
	}
	
	int rows = rs.GetRows();
	if (0 == rows) 
	{
		XERROR("%s: Query result is 0, QuerySql:%s",pModuleName ,pSQL);
		return 0;
	} 
	
	return rows;
}






int CMgrBase::UpdateSingleRow(const char *pSQL,const char *pModuleName)
{
	XINFO("%s,UpdateSingleRow: SQL:[%s]\n",pModuleName ,pSQL );
	size_t rows = 0;
	rows = m_pDb->ExecSql(pSQL);
	if (m_pDb->IsError()) 
	{
		XERROR("%s: QuerySql:%s,con=%d|sql=%s",pModuleName ,m_pDb->GetMySqlErrMsg(), m_pDb->IsConnect(), pSQL);
		m_nFail = -1;
		return -1;
	}
	
	
	if( -1 == rows)
	{
		XERROR("%s: Update error:rows:%ld,SQL:[%s],Error:[%s] \n",pModuleName ,rows,pSQL , m_pDb->GetMySqlErrMsg());
		m_nFail = -1;
		return -1 ;
	}

	/*
	if(0 == rows)
	{
		XERROR("%s: Update zero row:rows:%ld,SQL:[%s]\n",pModuleName ,rows,pSQL );
		return -1 ;
	}
	*/

	if(1 < rows)
	{
		XERROR("%s: Update many rows:rows:%ld,SQL:[%s]\n",pModuleName ,rows,pSQL );
		return -1 ;
	}

	return 0;
}






double CMgrBase::GetTradeFee(const char *pCoinName)
{

	memset(m_sJson,0,JSONLEN);
	snprintf(m_sSQL,SQLLEN-1,"SELECT TradeFee from 	COIN  where CoinName='%s' ;",pCoinName);

	MySqlRecordSet rs;
	double fTradeFee = 0.0;
	if( 0 <= GetDataListFromDB(rs,m_sSQL,"GetTradeFee") )
	{
			fTradeFee = 0.0;
	}
		
	fTradeFee = atof(rs.GetFieldByID(0, 0).c_str());
	XINFO("Coin:%s Trade fee is:[%f]\n",pCoinName ,fTradeFee );
	
	return   fTradeFee ;

}





