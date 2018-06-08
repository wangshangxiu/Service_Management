

#include "LoginOutMgr.h"

static pthread_mutex_t _LoninoutReadLock;


int CLoginOutMgr::LoginOut(STConsumerData *pData)
{

	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("LoginOut json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("LoginOut:Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char* sessionid = it->value.GetString(); 

	
	if(((it = doc.FindMember("RequestID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("LoginOut:Get RequestID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1;
	}
	const char *pRequestID = it->value.GetString(); 
	
	
	memset(m_sSQL,0,SQLLEN);
	memset(m_sJson,0,JSONLEN);
	
	if(TAG_LOGIN == pData->nTag)
	{
		
		if(((it = doc.FindMember("username")) == doc.MemberEnd()) || !it->value.IsString()) 
		{
			XERROR("LoginOut:Get username is error, json:[%s] \n ",pData->sConsumerBuf);
			return -1;
		}
		const char* username = it->value.GetString(); 
		
		if(((it = doc.FindMember("loginpassword")) == doc.MemberEnd()) || !it->value.IsString()) 
		{
			XERROR("LoginOut:Get loginpassword is error, json:[%s] \n ",pData->sConsumerBuf);
			return -1;
		}
		const char* loginpassword = it->value.GetString(); 

		if(((it = doc.FindMember("srcip")) == doc.MemberEnd()) || !it->value.IsString()) 
		{
			XERROR("LoginOut:Get srcip is error, json:[%s] \n ",pData->sConsumerBuf);
			return -1;
		}
		const char* srcip = it->value.GetString(); 

		if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
		{
			XERROR("LoginOut:Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
			return -1;
		}
		int nWsId_now = it->value.GetInt(); 


		XINFO("WS-->ManagerSrv:Login,json:[%s], tag:[0x%x]\n",pData->sConsumerBuf,pData->nTag);
		
		char sRoutingKey[ROUTINGKEYLEN]={0};


		memset(m_sJson,0,JSONLEN);
		// 登录验证
		snprintf(m_sSQL,SQLLEN-1,"SELECT id, loginpassword from user WHERE username='%s' and loginpassword='%s';",username,loginpassword);
		MySqlRecordSet rs ;
		size_t rows  = GetDataListFromDB(rs,m_sSQL,"Login db check ");
		if(1 != rows)// 发回登录失败
		{
			XERROR("Login Query :%s,return row:%d,[%s]\n",m_sSQL,rows,m_pDb->GetMySqlErrMsg());
			snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"SessionID\":\"%s\",\"RequestID\":\"%s\",\"Status\":-1,\"WSID\":%d}",TAG_LONIN_RESP,sessionid,pRequestID,nWsId_now);
			snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId_now);
			m_pTnode->PublishToMQ( sRoutingKey, m_sJson,strlen(m_sJson));

			XINFO("ManagerSrv-->WS,Send Login fail msg; routingkey:[%s],json:[%s],recv json:[%s],Tag[0x%x]\n",sRoutingKey,m_sJson,pData->sConsumerBuf,pData->nTag);
			
			return -1;
		}
		 
		long nUserId =  atol(rs.GetFieldByID(0, 0).c_str()) ;
		// 记录登录历史
		memset(m_sSQL,0,SQLLEN);
		time_t nowtime = time(NULL) ;
		snprintf(m_sSQL,SQLLEN-1,"INSERT INTO LOGIN_HISTORY(UserID,LoginTime,LoginIp) VALUE(%ld,%ld,'%s') ;", nUserId ,nowtime , srcip );
		m_pDb->ExecSql(m_sSQL);
		
		
		// 获取已经登录的wsid
		string _sUserId = std::to_string(nUserId);
		string sWsId_before = m_pRedis->HGet(_sUserId.c_str(),"WSID");
		string sSession_before = m_pRedis->HGet(_sUserId.c_str(),"SessionID");
		
		// 设置新的wsid
		string sWsId_now = std::to_string(nWsId_now);
		m_pRedis->HSet(_sUserId.c_str(),"WSID",sWsId_now.c_str());
		m_pRedis->HSet(_sUserId.c_str(),"SessionID",sessionid);
		
		
		if(!sWsId_before.empty()) //该用户已经登录,发出踢人消息
		{
            snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"SessionID\":\"%s\",\"RequestID\":\"%s\",\"Status\":-2,\"WSID\":%s}",TAG_LOGIN_KICK,sSession_before.c_str(),pRequestID,sWsId_before.c_str());
			snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%s",RTKEY_TOWEBSRV,sWsId_before.c_str());
			m_pTnode->PublishToMQ( sRoutingKey, m_sJson,strlen(m_sJson));

			XINFO("ManagerSrv-->WS, User:%s, UserId:%s alread online, and kick it; routingkey:[%s],json:[%],recv json:[%s],Tag[0x%x]\n",
				username , _sUserId.c_str(),sRoutingKey,m_sJson,pData->sConsumerBuf,pData->nTag);
			
		}

		
		snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"SessionID\":\"%s\",\"RequestID\":\"%s\",\"UserID\":%ld,\"Status\":0,\"WSID\":%d}",TAG_LONIN_RESP,sessionid,pRequestID,nUserId,nWsId_now);
		snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId_now);
		m_pTnode->PublishToMQ( sRoutingKey, m_sJson,strlen(m_sJson));

		XINFO("ManagerSrv-->WS,Send Login ack msg; routingkey:[%s],json:[%],recv json:[%s],Tag[0x%x]\n",sRoutingKey,m_sJson,pData->sConsumerBuf,pData->nTag);
		
		return 1;
		
		
		
	}
	else if (TAG_LOGOUT == pData->nTag)
	{
		XINFO("WS-->ManagerSrv: Logout,json:[%s]\n",pData->sConsumerBuf);
		if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
		{
			XERROR("LoginOut:UserId  is error, json:[%s] \n ",pData->sConsumerBuf);
			return -1;
		}
		long nUserId = it->value.GetInt64(); 
		
		string _sUserId = std::to_string(nUserId);
		m_pRedis->Del(_sUserId.c_str());

		
		return 0;
	}
	else
	{
		XERROR("LoginOut: tag:[%d] is error, data:[%s]\n",pData->nTag, pData->sConsumerBuf);
		return -1;
	}
	
	
	return 0;
}

void *LoginOutProcess(void *argv)
{
	CLoginOutMgr loginOutMgr;
	if(loginOutMgr.Init())return NULL;
	

	while(1)
	{
		pthread_mutex_lock(&_LoninoutReadLock);
		STConsumerData *pData = (STConsumerData *)g_pLoginoutDataPool->GetDataNode();
		pthread_mutex_unlock(&_LoninoutReadLock);
		if(NULL == pData)
		{
			usleep(5000);
			continue;
		}
		
		loginOutMgr.LoginOut(pData);
		
        //g_pLoginoutDataPool->PushUsedNode(pData);
        g_pMemNodePool->PushUsedNode(pData);
		
	}
	
	
	return NULL;
}


int LoginOutModule()
{
	pthread_mutex_init(&_LoninoutReadLock,NULL);
	
	//for(int i=0;i<g_stConfig.nLoginOutModuleNum;i++)
	{
		pthread_attr_t attr;
		pthread_attr_init (&attr);   
		pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);   
		pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);  
		
		pthread_t nThreadID;
		
		if (0 != pthread_create(&nThreadID, &attr, LoginOutProcess, NULL) )
		{
			XERROR("pthread_create LoginOutModule error:%s\n",strerror(errno));
			return -1;
		}
	}
	
	return 0;
}


