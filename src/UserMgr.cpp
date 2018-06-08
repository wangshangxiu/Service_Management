
#include "UserMgr.h"



 
int CUserMgr::SendSMS(STConsumerData * pData)
{
	XINFO("Send SMScode Request,json:%s",pData);

	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("SendSMS json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("SendSMS:Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId = it->value.GetInt64(); 


	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPassword: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("ModifyPassword: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWSID = it->value.GetInt() ; 

	if(((it = doc.FindMember("RequestID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPassword: Get RequestID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pRequestID = it->value.GetString() ; 

	
	char sContent[EMAILLEN]={0};
	

	XINFO("WS-->Manager: SendSMS/Mail: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);
	
	if(TAG_SMSCODEREQ_MODIFYPHONE == pData->nTag)
	{
		if(((it = doc.FindMember("MobilePhone")) == doc.MemberEnd()) || !it->value.IsString()) 
		{
			XERROR("SendSMS:Get MobilePhone is error, json:[%s] \n ",pData->sConsumerBuf);
			return -1 ;
		}
		strncpy(sContent, it->value.GetString(),EMAILLEN-1 ); 
	}
	else if (TAG_SMSCODEREQ == pData->nTag)
	{
		memset(m_sSQL,0,SQLLEN);
		snprintf(m_sSQL,SQLLEN,"Select MobilePhoneNum from User where ID = %ld;",nUserId);		
		
		MySqlRecordSet rs ;
		size_t rows  = GetDataListFromDB(rs,m_sSQL,"ManagerMudule");
		if(rows != 1)
		{
			XERROR("SendSMS:Query email/phone is error, rows:[%d],SQL:[%ld] \n ",rows,m_sSQL  );
			return -1;
		}
		strncpy(sContent,  rs.GetFieldByID(0, 0).c_str(),EMAILLEN-1 );
	}
	else
	{
		XERROR("SendSMS:Tag is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	
	char sRandomCode[RANDOMCODELEN]={0};
	GenerateRandomCode(sRandomCode);
	
	m_pRedis->HSet(sContent,"code",sRandomCode);
	//m_pRedis->HSet(sContent,"code","abcd1234");
	m_pRedis->HSet(sContent,"time",std::to_string(time(NULL)).c_str());


	

	memset(m_sJson,0,JSONLEN);
	//snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"MobilePhone\":\"%s\",\"Code\":\"%s\"}",TAG_SENDSMSCODE,nUserId,sContent, sRandomCode);
	snprintf(m_sJson,JSONLEN-1,"{\"tag\":%d,\"dest\":%s,\"code\":\"%s\",\"sessionID\":\"%s\",\"requestID\":\"%s\",\"wSID\":%d}",
	TAG_SENDSMSCODE,sContent,sRandomCode,pSessionId,pRequestID,nWSID);
	
	
	m_pTnode->PublishToMQ(g_stConfig.mq_routingkey_tosmssender.c_str(), m_sJson,strlen(m_sJson));

	XINFO("ManagerSrv-->SMSMailSender, routingkey:[%s], freesz json:[%s],Tag[0x%x]\n",g_stConfig.mq_routingkey_tosmssender.c_str(),m_sJson,TAG_SENDSMSCODE);
	
	return 0;
}





int CUserMgr::ModifyPassword(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("ModifyPassword json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;
	

	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPassword: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("ModifyPassword: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 

	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("ModifyPassword: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	

	if(((it = doc.FindMember("OldPwd")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPassword: Get OldPwd is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pOldPwd = it->value.GetString() ; 
	

	if(((it = doc.FindMember("NewPwd")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPassword: Get NewPwd is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pNewPwd = it->value.GetString() ; 
		
	if(((it = doc.FindMember("SmsCode")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPassword: Get SmsCode is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSmsCode = it->value.GetString() ; 

	
	XINFO("WS-->Manager: ModifyPassword: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);

	//1. 插入手机号码
	memset(m_sSQL,0,SQLLEN);
	int status = -1;
	int nTag_resp ;
	if(TAG_MODIFYLOGINPWD == pData->nTag)
	{
		snprintf(m_sSQL,SQLLEN-1,"Select MobilePhoneNum from user where id=%ld and loginPassword='%s' ; ", nUserId,pOldPwd);
		nTag_resp = TAG_MODIFYLOGINPWD_RESP ;
	}
	else
	{
		snprintf(m_sSQL,SQLLEN-1,"Select MobilePhoneNum from user where id=%ld and TradePassword='%s' ; ", nUserId,pOldPwd);
		nTag_resp = TAG_MODIFYTRADEPWD_RESP ;
	}

	MySqlRecordSet rs ;
	status = GetDataListFromDB(rs,m_sSQL,"ModifyPassword");
	if(1 == status)
	{
		const char *pMobilePhone = rs.GetFieldByID(0, 0).c_str();
		status = CheckCode(pMobilePhone,pSmsCode,pData->sConsumerBuf );
		if(0 == status)
		{
			memset(m_sSQL,0,SQLLEN);
			if(TAG_MODIFYLOGINPWD == pData->nTag)
			{
				snprintf(m_sSQL,SQLLEN-1,"UPDATE User set loginPassword='%s' where ID=%ld ;",pNewPwd,nUserId);
			}
			else
			{
				snprintf(m_sSQL,SQLLEN-1,"UPDATE User set TradePassword='%s' where ID=%ld ;",pNewPwd,nUserId);
			}
			
			status = UpdateSingleRow(m_sSQL,"ModifyPassword");
		}
		
	}
	else
	{
		status = -1;
	}
	
	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"SessionID\":\"%s\",\"Status\":\"%d\"}",nTag_resp,pSessionId,status );
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	
	m_pTnode->PublishToMQ(sRoutingKey,m_sJson,strlen(m_sJson));

	XINFO("ManagerSrv-->WS, ModifyPassword, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);

	
	return 0;
}








int CUserMgr::ModifyPhone(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("ModifyPhone json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;


	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPhone: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("ModifyPhone: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 
	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("ModifyPhone: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("Email")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPhone: Get EMail is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pEmail = it->value.GetString() ; 
	
	
	if(((it = doc.FindMember("MobilePhone")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPhone: Get MobilePhone is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pMobilePhone = it->value.GetString() ; 
		
	if(((it = doc.FindMember("SmsCode")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ModifyPhone: Get SmsCode is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSmsCode = it->value.GetString() ; 

	XINFO("WS-->Manager: ModifyPhone: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);
	
	
	//int status = CheckCode(pMobilePhone,pSmsCode,pData->sConsumerBuf,m_pRedis);
	int status = CheckCode(pMobilePhone,"abcd1234",pData->sConsumerBuf);
	if(0 == status)
	{
		snprintf(m_sSQL,SQLLEN-1,"UPDATE User set MobilePhoneNum='%s' where ID=%ld and UserName = '%s' ;",
					pMobilePhone,nUserId, pEmail);
		status = UpdateSingleRow(m_sSQL,"ModifyPhone");
		
	}
	XINFO("ModifyPhone,json:%s",pData);
	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"SessionID\":\"%s\",\"Status\":\"%d\"}",TAG_MODIFYMOBILEPHONE_RESP,pSessionId,status );
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	
	m_pTnode->PublishToMQ(sRoutingKey,m_sJson,strlen(m_sJson));	

	XINFO("ManagerSrv-->MS,ModifyPhone, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);
	
	return 0;
}



int CUserMgr::DepositAddrRequest(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("DepositAddrRequest json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("DepositAddrRequest: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("DepositAddrRequest: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("DepositAddrRequest: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 
	
	if(((it = doc.FindMember("CoinName")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("DepositAddrRequest: Get CoinName is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *sCoinName = it->value.GetString() ; 

	XINFO("WS-->Manager: DepositAddrRequest: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);

	
	memset(m_sJson,0,JSONLEN);
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL,SQLLEN,"select DeAdress from user_depositaddress where UserID = %ld and BlockChainID = \
	(select BlockChainID from Coin where CoinName='%s') ;",nUserId,sCoinName);		
	MySqlDB db ;
	MySqlRecordSet rs ;
	size_t rows  = GetDataListFromDB(rs,m_sSQL,"DepositAddrRequest");
	if(rows != 1)
	{
		XERROR("DepositAddrRequest:Query deposit address e is error, rows:[%d],SQL:[%ld] \n ",rows,m_sSQL  );
		snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\",\"DepsositAddr\":\"address error\"}",
				TAG_DEPOSITADDR_RESP,nUserId,pSessionId );
	}
	else
	{
		const char *pDeAdress = rs.GetFieldByID(0,0).c_str();
		snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\",\"DepsositAddr\":\"%s\"}",
				TAG_DEPOSITADDR_RESP,nUserId,pSessionId,pDeAdress );
	}	
	
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	m_pTnode->PublishToMQ(sRoutingKey,m_sJson,strlen(m_sJson));

	XINFO("ManagerSrv-->MS,DepositAddrRequest, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);
	
	return 0;	
	
}





int CUserMgr::AddWithDrawAddr(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("AddWithDrawAddr json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("AddWithDrawAddr: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddWithDrawAddr: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("AddWithDrawAddr: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 
	
	if(((it = doc.FindMember("CoinName")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddWithDrawAddr: Get CoinName is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *sCoinName = it->value.GetString() ;
	
	if(((it = doc.FindMember("WithdrawAddr")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddWithDrawAddr: Get WithdrawAddr is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pWithdrawAddr = it->value.GetString() ; 
	
	/*
	if(((it = doc.FindMember("MobilePhone")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddWithDrawAddr: Get MobilePhone is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *sMobilePhone = it->value.GetString() ;
	*/

	/*
	if(((it = doc.FindMember("SmsCode")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddWithDrawAddr: Get SmsCode is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSmsCode = it->value.GetString() ;  
	*/
	
	if(((it = doc.FindMember("Note")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddWithDrawAddr: Get Note is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pNote = it->value.GetString() ;

	
	
	XINFO("WS-->Manager: AddWithDrawAddr: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);


	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL,SQLLEN-1,"Select MobilePhoneNum from user where id=%ld; ", nUserId);
	MySqlRecordSet rs ;
	int status = GetDataListFromDB(rs,m_sSQL,"AddWithDrawAddr");
	if(1 == status)
	{
		const char *pMobilePhone = rs.GetFieldByID(0, 0).c_str();
		//status = CheckCode(pMobilePhone,pSmsCode,pData->sConsumerBuf );
		// for test
		const char *pSmsCode = "123e1234";
		status = CheckCode(pMobilePhone,pSmsCode,pData->sConsumerBuf );
		if(0 == status)
		{
			memset(m_sSQL,0,SQLLEN);
			snprintf(m_sSQL,SQLLEN-1,"INSERT INTO USER_WITHDRAWADDRESS(UserID,CoinID,WithDrawAdress,Note)\
			VALUE(%ld,(select id from Coin where CoinName='%s'),'%s','%s')",
			nUserId,sCoinName,pWithdrawAddr,pNote );
			
			status = UpdateSingleRow(m_sSQL,"ModifyPassword");
		}
		
	}
	else
	{
		status = -1;
	}


	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\",\"Status\":%d}", 
			TAG_ADDWITHDRWADDR_RESP,nUserId,pSessionId,status );
	
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	m_pTnode->PublishToMQ(sRoutingKey ,m_sJson,strlen(m_sJson));
	
	XINFO("ManagerSrv-->MS,AddWithDrawAddr, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);
	return 0;
}

int CUserMgr::DelWithdrawAddr(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("DelWithdrawAddr json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("DelWithdrawAddr: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("DelWithdrawAddr: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("DelWithdrawAddr: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 
	
	if(((it = doc.FindMember("CoinName")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("DelWithdrawAddr: Get CoinName is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *sCoinName = it->value.GetString() ;
	
	if(((it = doc.FindMember("WithdrawAddr")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("DelWithdrawAddr: Get WithdrawAddr is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pWithdrawAddr = it->value.GetString() ; 

	XINFO("WS-->Manager: DelWithdrawAddr: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);
	
	
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL,SQLLEN-1,
		"DELETE FROM USER_WITHDRAWADDRESS WHERE UserID = '%ld' and WithDrawAdress = '%s';",
		nUserId,pWithdrawAddr);
	
	int status = 0;
	status = UpdateSingleRow(m_sSQL,"DelWithdrawAddr");
	
	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\",\"Status\":%d}", 
			TAG_DELWITHDRWADDR_RESP,nUserId,pSessionId,status );
	
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson,strlen(m_sJson));	

	XINFO("ManagerSrv-->MS,DelWithdrawAddr, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);

	
	return 0;
}


int CUserMgr::GetBankList(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("GetBankList json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("GetBankList: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("GetBankList: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("GetBankList: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 
	
	XINFO("WS-->Manager: GetBankList: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);
	

	MySqlRecordSet rs ;
	const char *pSQL = "SELECT ID,BankName from BANK_LIST where Status = 1;" ;
	size_t rows =  GetDataListFromDB(rs,pSQL,"GetBankList");
	if(rows <= 0)
	{
		XERROR("GetBankList: Bank list is empty, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	
	int nBankId = atol(rs.GetFieldByID(0, 0).c_str());
	const char *pBankName = rs.GetFieldByID(0, 1).c_str();	
	
	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\",\"BankList\":[{\"BankId\":%d,\"BankName\":\"%s\"}", 
			TAG_DELBANKACCOUNT_RESP,nUserId,pSessionId,nBankId,pBankName );
	
	
	//  参考 GetBankAccountList
	//  ......
	

	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson,strlen(m_sJson));		
	
	XINFO("ManagerSrv-->MS,GetBankList, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);
	
	return 0;
}



int CUserMgr::GetBankAccountList(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("GetBankAccountList json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("GetBankAccountList: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("GetBankAccountList: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("GetBankAccountList: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 
	

	XINFO("WS-->Manager: GetBankAccountList: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);
	
	MySqlRecordSet rs ;
	
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL , SQLLEN-1, "Select ID, HolderName, BankName, BankAccount, OpenBranchBank from USER_BANKLIST  where  UserID=%ld ", nUserId  );
	size_t rows =  GetDataListFromDB(rs,m_sSQL,"GetBankAccountList");
	if(rows <= 0)
	{
		XERROR("GetBankAccountList: list is empty, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	
	const char *pId = rs.GetFieldByID(0, 0).c_str();
	const char *pHolderName = rs.GetFieldByID(0, 1).c_str();	
	const char *pBankName = rs.GetFieldByID(0, 2).c_str();	
	const char *pBankAccout = rs.GetFieldByID(0, 3).c_str();
	const char *pOpenBranchBank = rs.GetFieldByID(0, 4).c_str();
	
	
	char m_sJson_big[BANKACCOUNTLIST];
	memset(m_sJson_big,0,BANKACCOUNTLIST);
	snprintf(m_sJson_big,BANKACCOUNTLIST-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\",\"BankList\":\
		[{\"AccountID\":%s,\"BankName\":\"%s\",\"CardNumber\":\"%s\",\"OpenBank\":\"%s\",\"UserName\":\"%s\"}", 
			TAG_GETBANKACCOUNTLIST_RESP,nUserId,pSessionId,pId,pBankName, pBankAccout,pOpenBranchBank, pHolderName );

	int ret = -1;	
	if(1 < rows)
	{
		
		for(size_t i=1; i < rows ; i++)
		{
			size_t nCurLen = strlen(m_sJson_big) ;
		
			const char *pId = rs.GetFieldByID(i, 0).c_str();
			const char *pHolderName = rs.GetFieldByID(i, 1).c_str();	
			const char *pBankName = rs.GetFieldByID(i, 2).c_str();	
			const char *pBankAccout = rs.GetFieldByID(i, 3).c_str();
			const char *pOpenBranchBank = rs.GetFieldByID(i, 4).c_str();
			
			if((BANKACCOUNTLIST - nCurLen) < (1+20+ strlen(pId) + strlen(pHolderName) + strlen(pBankName)+strlen(pBankAccout)+strlen(pOpenBranchBank) +1) )
			{
				ret = -1;
				break;
			}
			
			sprintf(m_sJson_big+nCurLen,
				",{\"AccountID\":%s,\"BankName\":\"%s\",\"CardNumber\":\"%s\",\"OpenBank\":\"%s\",\"UserName\":\"%s\"}",
				pId,pBankName, pBankAccout,pOpenBranchBank, pHolderName);
			ret = 0;
		}
	}
	
	if(  1 == rows || ((-1 != ret) && (BANKACCOUNTLIST - strlen(m_sJson_big) > 3 )))
	{
		strcat(m_sJson_big,"]}");
	}
	else
	{
		XERROR("GetBankAccountList: Bank list is too many ,but buffer size is too small, json:[%s] \n ",m_sJson_big);
		return -1;
	}
		
	
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson_big,strlen(m_sJson_big));		
	XINFO("ManagerSrv-->MS,GetBankAccountList, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson_big,pData->nTag);
	
	return 0;
}


int CUserMgr::AddBankAccount(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("AddBankAccount json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("AddBankAccount: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddBankAccount: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("AddBankAccount: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt(); 
	
	
	if(((it = doc.FindMember("BankName")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddBankAccount: Get BankName is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pBankName = it->value.GetString() ; 
	
	
	if(((it = doc.FindMember("BankAccout")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddBankAccount: Get BankAccout is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pBankAccout = it->value.GetString() ; 	
	
	
	if(((it = doc.FindMember("HolderName")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddBankAccount: Get HolderName is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pHolderName = it->value.GetString() ; 	
	
	if(((it = doc.FindMember("OpenBranchBank")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AddBankAccount: Get OpenBranchBank is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pOpenBranchBank = it->value.GetString() ; 

	XINFO("WS-->Manager: AddBankAccount: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);
	
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL,SQLLEN-1,
		"INSERT INTO user_banklist(UserID,HolderName,BankName,BankAccount,OpenBranchBank,Status)\
		VALUE(%ld,'%s','%s','%s','%s',1) ;",
		nUserId,pHolderName, pBankName ,pBankAccout, pOpenBranchBank);
	int status = UpdateSingleRow(m_sSQL,"AddBankAccount");
	
	
	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\",\"Status\":%d}", 
			TAG_ADDBANKACCOUNT_RESP,nUserId,pSessionId,status );
	
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	m_pTnode->PublishToMQ(sRoutingKey,m_sJson,strlen(m_sJson));

	XINFO("ManagerSrv-->MS,AddBankAccount, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);
	
	return 0;
}


int CUserMgr::DelBankAccount(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("DelBankAccount json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("DelBankAccount: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("DelBankAccount: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("DelBankAccount: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt(); 
	
	/*
	if(((it = doc.FindMember("BankId")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("DelBankAccount: Get BankId is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nBankId =  it->value.IsInt() ; 
	*/
	
	if(((it = doc.FindMember("BankAccount")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("DelBankAccount: Get BankAccount is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pBankAccout = it->value.GetString() ; 

	XINFO("WS-->Manager: DelBankAccount: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);

	
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL,SQLLEN-1,
		"DELETE FROM user_banklist WHERE UserID = %ld and  BankAccount = '%s';",
		nUserId,pBankAccout );
	
	int status = UpdateSingleRow(m_sSQL,"DelBankAccount");
	
	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\",\"Status\":%d}", 
			TAG_DELBANKACCOUNT_RESP,nUserId,pSessionId,status );
	
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	m_pTnode->PublishToMQ(sRoutingKey ,m_sJson,strlen(m_sJson));
	XINFO("ManagerSrv-->MS,DelBankAccount, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);	
	
	return 0;
}



int CUserMgr::KYCResourceCommint(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("KYCResourceCommint json parse error ,data:[%s]", pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("KYCResourceCommint: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("KYCResourceCommint: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("KYCResourceCommint: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt(); 

	

	if(((it = doc.FindMember("country")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("KYCResourceCommint: Get country is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pCountry = it->value.GetString() ; 


	if(((it = doc.FindMember("firstName")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("KYCResourceCommint: Get firstName is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pFirstName = it->value.GetString() ; 


	if(((it = doc.FindMember("lastName")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("KYCResourceCommint: Get lastName is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pLastName = it->value.GetString() ; 


	if(((it = doc.FindMember("idType")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("KYCResourceCommint: Get idType is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pIdType = it->value.GetString() ; 


	if(((it = doc.FindMember("idNumber")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("KYCResourceCommint: Get idNumber is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pIdNumber = it->value.GetString() ; 



	if(((it = doc.FindMember("img1")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("KYCResourceCommint: Get img1 is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pImg1 = it->value.GetString() ; 


	if(((it = doc.FindMember("img2")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("KYCResourceCommint: Get img2 is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pImg2 = it->value.GetString() ; 


	if(((it = doc.FindMember("img3")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("KYCResourceCommint: Get img3 is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pImg3 = it->value.GetString() ; 


	XINFO("WS-->Manager: KYCResourceCommint: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);


	
	memset(m_sSQL, 0 ,SQLLEN);
	snprintf(m_sSQL,SQLLEN-1,"update User set IsKYC = 1 where id=%ld ; ", nUserId,nUserId);
	UpdateSingleRow(m_sSQL,"KYCResourceCommint");

	snprintf(m_sSQL,SQLLEN-1,"delete from USER_KYC where userid=%ld ;", nUserId,nUserId);
	UpdateSingleRow(m_sSQL,"KYCResourceCommint");
		

	memset(m_sSQL, 0 ,SQLLEN);
	snprintf(m_sSQL,SQLLEN-1,"INSERT INTO USER_KYC(UserId,country,firstName,Secondname,idType,Idno,Image1,Image2,Image3)\
		value(%ld,'%s','%s','%s','%s','%s','%s','%s','%s');"
		,nUserId,pCountry,pFirstName,pLastName, pIdType,pIdNumber,pImg1,pImg2,pImg3 );

	int rows = UpdateSingleRow(m_sSQL,"KYCResourceCommint");


	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"SessionID\":\"%s\",\"Status\":%d}",TAG_KYCCommint_RESP,pSessionId,rows );
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson,strlen(m_sJson));
	XINFO("ManagerSrv-->WS,KYCResourceCommint, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);
	return 0;
}





int CUserMgr::UserRegister(STConsumerData *pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("UserRegister json parse error ,data:[%s]", pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	


	if(((it = doc.FindMember("Email")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("UserRegister: Get Email is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pEmail = it->value.GetString() ; 
	
	
	if(((it = doc.FindMember("Password")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("UserRegister: Get Password is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pPassword = it->value.GetString() ; 



	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("UserRegister: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	


	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("UserRegister: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt(); 


	XINFO("WS-->Manager: UserRegister: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);

	memset(m_sSQL, 0 ,SQLLEN);
	snprintf(m_sSQL,SQLLEN-1,"INSERT INTO USER(UserName,loginPassword,RegisterTime,UserType,Status,TradePassword) value('%s','%s',sysdate(),0,1,'%s') ;",
		pEmail, pPassword,pPassword
	);
	
	size_t nUserId = -1 ;
	int rows = UpdateSingleRow(m_sSQL,"UserRegister");
	if( 0 == rows )
	{
		// 获取ID
		nUserId = m_pDb->GetInsertID();
		if(-1 == nUserId)
		{
			XERROR("UserRegister: Get nUserID is error, json:[%s] \n ",pData->sConsumerBuf);
			nUserId = -1;
			rows = 0 ;
			return -1 ;
		}

		//build test data for test
		memset(m_sSQL, 0 ,SQLLEN);
		snprintf(m_sSQL,SQLLEN-1,"insert  into user_assets\
		(UserID,CoinID,Total,Available,Frozen,Valuation) \
		values \
		(%ld,1,'100000.000000000','100000.000000000','0.000000000','100000.000000000'),\
		(%ld,2,'100000.000000000','100000.000000000','0.000000000','100000.000000000'),\
		(%ld,3,'100000.000000000','100000.000000000','0.000000000','100000.000000000'),\
		(%ld,4,'200000.000000000','200000.000000000','0.000000000','200000.000000000'),\
		(%ld,5,'300000.000000000','300000.000000000','0.000000000','300000.000000000'),\
		(%ld,6,'400000.000000000','400000.000000000','0.000000000','400000.000000000'),\
		(%ld,7,'400000.000000000','400000.000000000','0.000000000','400000.000000000'); " 
		,nUserId,nUserId,nUserId,nUserId,nUserId,nUserId,nUserId);
		UpdateSingleRow(m_sSQL,"UserRegister build test data");


		//build test data for test
		memset(m_sSQL, 0 ,SQLLEN);
		snprintf(m_sSQL,SQLLEN-1,"INSERT INTO user_withdrawaddress(UserID,CoinID,WithDrawAdress) \
		VALUES (%ld, 1, 'yurtuy6fFE5qtaXEEBiRcQV1Qmhs9aqtywy1'),\
		(%ld, 2, 'qerqe6fFE5qtaXEEBiRcQV1Qmhs9rtwert2'),\
		(%ld, 3, 'edrqewy6fFE5qtaXEEBiRcQV1oQmhswrtw3'),\
		(%ld, 4, 'rggqny6fFE5qtaXEEBiRcQVo1Qmhs9wrtw4'),\
		(%ld, 5, 'qewrny6fFE5qtaXEEBiRc1Qmhs9asfdgsa5'),\
		(%ld, 6, 'qreqn6fFE5qtaXEEBiRcQV1Qmhs9afttjh6'),\
		(%ld, 7, 'adsfkny6fFE5qtaXEEBiRcQV1Qmhswetyw7'); ",
		nUserId,nUserId,nUserId,nUserId,nUserId,nUserId,nUserId
		);
		UpdateSingleRow(m_sSQL,"UserRegister build test data");
		

		//build test data for test
		memset(m_sSQL, 0 ,SQLLEN);
		snprintf(m_sSQL,SQLLEN-1,"\	
			INSERT INTO user_depositaddress \
			(UserID, BlockChainID, DeAdress, PrivateKey)\
			VALUES \
			(%ld, 1, 'mysicyqaaSCAWB9h9BxzYsdfadVa84NZDS', 'sdADSFafaASDFsdrfqewrqASDF2ewrtrewyNJASDF'),\
			(%ld, 2, '0xff0c195cb7754f355dece7dd446d07be5b5327d6', 'TvGnf53QU2EljRNnSJITb3VHnCu3MRkv');" 
			, nUserId,nUserId );
		
		UpdateSingleRow(m_sSQL,"UserRegister build test data");
		

		// end 
		
	}


	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d, \"UserID\":%ld, \"SessionID\":\"%s\",\"Status\":\"%d\"}",TAG_REGISTER_RESP,nUserId,pSessionId,rows );
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson,strlen(m_sJson));
	XINFO("ManagerSrv-->MS,UserRegister, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);

	return 0;
}



int CUserMgr::ForgetLoginPwd(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("ForgetLoginPwd json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;
	
	if(((it = doc.FindMember("EMail")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ForgetLoginPwd: Get EMail is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pEmail = it->value.GetString() ; 
	
	if(((it = doc.FindMember("Password")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ForgetLoginPwd: Get Password is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pPassword = it->value.GetString() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("ForgetLoginPwd: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("ForgetLoginPwd: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 

	XINFO("WS-->Manager: ForgetLoginPwd: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);

	
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL,SQLLEN-1,"UPDATE User set loginPassword='%s' where UserName = '%s';", pPassword,pEmail);
	
	
	int status = UpdateSingleRow(m_sSQL,"ModifyLoginPwd");
	if(status == 1)
	{	status=0; }
	else
	{	status = -100;}
	
	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"SessionID\":\"%s\",\"Status\":%d}",TAG_FORGETLOGINPWD_RESP,pSessionId,status );
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson,strlen(m_sJson));	
	XINFO("ManagerSrv-->MS,ForgetLoginPwd, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson,pData->nTag);
	
	return 0;
}




int CUserMgr::GetWithdrawAddrList(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("GetWithdrawAddrList json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("GetWithdrawAddrList: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 


	if(((it = doc.FindMember("CoinName")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("GetWithdrawAddrList: Get CoinName is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pCoinName = it->value.GetString() ; 

	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("GetWithdrawAddrList: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("GetWithdrawAddrList: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 
	

	XINFO("WS-->Manager: GetWithdrawAddrList: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);



	
	MySqlRecordSet rs ;
	
	memset(m_sSQL,0,SQLLEN);
	if(0 == strcmp(pCoinName,"ALL"))
	{
		snprintf(m_sSQL , SQLLEN-1, "SELECT a.WithDrawAdress,a.Note, b.coinname FROM USER_WITHDRAWADDRESS a,coin b where  a.UserID=%ld and a.CoinId = b.id;", nUserId  );
	}
	else
	{
		snprintf(m_sSQL , SQLLEN-1, "SELECT a.WithDrawAdress,a.Note, b.coinname FROM USER_WITHDRAWADDRESS a,coin b where  a.UserID=%ld and a.CoinId = b.id and b.coinname='%s';", nUserId ,pCoinName );
	}
	
	
	size_t rows =  GetDataListFromDB(rs,m_sSQL,"GetWithdrawAddrList");
	if(rows <= 0)
	{
		XERROR("GetWithdrawAddrList: list is empty, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	
	const char *pAddr = rs.GetFieldByID(0, 0).c_str();
	const char *pNote = rs.GetFieldByID(0, 1).c_str();	
	const char *pCoinName2 = rs.GetFieldByID(0, 2).c_str();	
	
	char m_sJson_big[BANKACCOUNTLIST];
	memset(m_sJson_big,0,BANKACCOUNTLIST);
	snprintf(m_sJson_big,BANKACCOUNTLIST-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\",\"AddrList\":\
		[{\"CoinName\":\"%s\",\"addr\":\"%s\",\"Notes\":\"%s\"}", 
			TAG_GETWITHDRAWADDR_RESP,nUserId,pSessionId, pCoinName2 , pAddr,pNote);

	int ret = -1;	
	if(1 < rows)
	{
		
		for(size_t i=1; i < rows ; i++)
		{
			size_t nCurLen = strlen(m_sJson_big) ;
		
			const char *pAddr = rs.GetFieldByID(i, 0).c_str();
			const char *pNote = rs.GetFieldByID(i, 1).c_str();
			const char *pCoinName2 = rs.GetFieldByID(i, 2).c_str();	
			if((BANKACCOUNTLIST - nCurLen) < (1+20+ strlen(pAddr) + strlen(pNote)  +1) )
			{
				ret = -1;
				break;
			}
			
			sprintf(m_sJson_big+nCurLen,",{ \"CoinName\":\"%s\" ,\"addr\":\"%s\",\"Notes\":\"%s\"}",pCoinName2,pAddr,pNote);
			ret = 0;
		}
	}
	
	if(  1 == rows || ((-1 != ret) && (BANKACCOUNTLIST - strlen(m_sJson_big) > 3 )))
	{
		strcat(m_sJson_big,"]}");
	}
	else
	{
		XERROR("GetWithdrawAddrList: withdraw list is too many ,but buffer size is too small, json:[%s] \n ",m_sJson_big);
		return -1;
	}
		
	
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson_big,strlen(m_sJson_big));		
	XINFO("ManagerSrv-->MS,GetWithdrawAddrList, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson_big,pData->nTag);
	
	return 0;
}




int CUserMgr::GetKYCResource(STConsumerData * pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("GetKYCResource json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	rapidjson::Value::ConstMemberIterator it;	
	
	if(((it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("GetKYCResource: Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId =  it->value.GetInt64() ; 
	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("GetKYCResource: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("GetKYCResource: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt() ; 


	if(((it = doc.FindMember("RequestID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("GetKYCResource: Get RequestID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pRequestID = it->value.GetString() ; 
	

	XINFO("WS-->Manager: GetKYCResource: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);

	
	MySqlRecordSet rs ;
	
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL , SQLLEN-1, "Select Country,Firstname, Secondname, Idtype, Idno, Image1, Image2, Image3 from user_kyc  where  UserID=%ld ", nUserId  );
	size_t rows =  GetDataListFromDB(rs,m_sSQL,"GetKYCResource");
	if(rows <= 0)
	{
		XERROR("GetKYCResource: list is empty, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}


	const char *Country = rs.GetFieldByID(0, 0).c_str();
	const char *Firstname = rs.GetFieldByID(0, 1).c_str();
	const char *Secondname = rs.GetFieldByID(0, 2).c_str();
	const char *Idtype = rs.GetFieldByID(0, 3).c_str();
	const char *Idno = rs.GetFieldByID(0, 4).c_str();
	const char *Image1 = rs.GetFieldByID(0, 5).c_str();
	const char *Image2 = rs.GetFieldByID(0, 6).c_str();
	const char *Image3 = rs.GetFieldByID(0, 7).c_str();

	
	char m_sJson_big[JSONLEN];
	memset(m_sJson_big,0,JSONLEN);
	snprintf(m_sJson_big,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"SessionID\":\"%s\", \"RequestID\":\"%s\",\
	\"Country\":\"%s\",\"Firstname\":\"%s\", \"Secondname\":\"%s\", \"Idtype\":\"%s\", \"Idno\":\"%s\", \"Image1\":\"%s\", \"Image2\":\"%s\", \"Image3\":\"%s\" }"  , 
			TAG_QUERYKYC_RESP,nUserId,pSessionId,pRequestID, 
			Country, Firstname,Secondname ,Idtype, Idno, Image1,Image2,Image3);


	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson_big,strlen(m_sJson_big));		
	XINFO("ManagerSrv-->WS,GetKYCResource, routingkey:[%s], json:[%s],tag[0x%x]\n",sRoutingKey,m_sJson_big,pData->nTag);
				
	return 0;

}






void *ManagerProcess(void *agrv)
{
	CUserMgr UserMgr;
	if(UserMgr.Init())return NULL;
	
	
	while(1)
	{
		STConsumerData * pData = (STConsumerData *)g_pMgrDataPool->GetDataNode();
		if(NULL == pData)
		{
			usleep(5000);
			continue;
		}
		
		switch(pData->nTag)
		{
			case TAG_SMSCODEREQ :
			case TAG_SMSCODEREQ_MODIFYPHONE:
			{
				UserMgr.SendSMS(pData);
				break;
			}
			case TAG_MODIFYLOGINPWD:
			{
				UserMgr.ModifyPassword(pData);
				break;
			}	
			case TAG_MODIFYTRADEPWD:
			{
				UserMgr.ModifyPassword(pData);
				break;
			}
			case TAG_MODIFYMOBILEPHONE:
			{
				UserMgr.ModifyPhone(pData);
				break;
			}
			case TAG_DEPOSITADDR:
			{
				UserMgr.DepositAddrRequest(pData);
				break;
			}
			case TAG_ADDWITHDRWADDR:
			{
				UserMgr.AddWithDrawAddr(pData);
				break;
			}
			case TAG_DELWITHDRWADDR:
			{
				UserMgr.DelWithdrawAddr(pData);
				break;
			}
			case TAG_GETBANKACCOUNTLIST:
			{
				UserMgr.GetBankAccountList(pData);
				break;
			}
			case TAG_ADDBANKACCOUNT:
			{
				UserMgr.AddBankAccount(pData) ;
				break;
			}
			case TAG_DELBANKACCOUNT:
			{
				UserMgr.DelBankAccount(pData);
				break;
			}
			case TAG_KYCCommint:
			{
				UserMgr.KYCResourceCommint(pData);
				break;
			}
			case TAG_REGISTER:
			{
				UserMgr.UserRegister(pData);
				break;
			}
			case TAG_FORGETLOGINPWD:
			{
				UserMgr.ForgetLoginPwd(pData);
				break;
			}
			case TAG_GETWITHDRAWADDR:
			{
				UserMgr.GetWithdrawAddrList(pData);
				break;
			}
			case TAG_QUERYKYC:
			{
				UserMgr.GetKYCResource(pData);
				break;
			}
			default:
			{
				XERROR("ManagerProcess: don't support tag:0x%x, data:[%s]\n",pData->nTag, pData->sConsumerBuf);
				break;
			}
		}
		
        //g_pMgrDataPool->PushUsedNode(pData);
        g_pMemNodePool->PushUsedNode(pData);
		
	}
	
	return NULL;
}


int CreateManagerModule()
{

	pthread_attr_t attr;
	pthread_attr_init (&attr);   
	pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);   
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);  
		
	pthread_t nThreadID;
		
	if (0 != pthread_create(&nThreadID, &attr, ManagerProcess, NULL) )
	{
		XERROR("pthread_create AssetsModule error:%s\n",strerror(errno));
		return -1;
	}
	
	return 0;
}
