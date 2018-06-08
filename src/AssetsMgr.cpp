
#include "AssetsMgr.h"

pthread_mutex_t	 static _AssetsReadLock;


int CAssetsMgr::SendFailMsg(int nTag,const char *pSessionId,int nWsId, const char *pJson)
{
	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"SessionID\":\"%s\",\"Status\":-1}",nTag,pSessionId );
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson,strlen(m_sJson));

	XINFO("SendFailMsg --> WS-MQ ,routingkey:[%s], json:[%s], data:[%s],Tag[0x%x]\n",sRoutingKey, m_sJson , pJson,nTag);
	
	return 0;
}

int CAssetsMgr::AssetsTradeSucessProcess(STConsumerData *pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		//XERROR("AssetsTradeSucessProcess Freeze/UnfreezeProcess json parse error,data:[%s]",   pData->sConsumerBuf );   
		XERROR("AssetsTradeSucessProcess Freeze/UnfreezeProcess json parse error ,data:[%s]", pData->sConsumerBuf );   
		return -1;
	}
	
	rapidjson::Value::ConstMemberIterator it;
		

	if(((it = doc.FindMember("AskUserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("AssetsTradeSucessProcess: Get AskUserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nAskUserID =  it->value.GetInt64() ; 

	if(((it = doc.FindMember("BidUserID")) == doc.MemberEnd()) || !it->value.IsInt64()) 
	{
		XERROR("AssetsTradeSucessProcess: Get BidUserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nBidUserID =  it->value.GetInt64() ; 

	
	if(( (it = doc.FindMember("DealNum")) == doc.MemberEnd()) || !it->value.IsDouble())
	{
		XERROR("AssetsTradeSucessProcess :DealNum is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	double fDealNum = it->value.GetDouble();


	if(( (it = doc.FindMember("DealPrice")) == doc.MemberEnd()) || !it->value.IsDouble())
	{
		XERROR("AssetsTradeSucessProcess :DealPrice is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	double fDealPrice = it->value.GetDouble();

	if(((it = doc.FindMember("Symbol")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("AssetsTradeSucessProcess: Get Symbol is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSymbol = it->value.GetString() ; 


	XINFO("WS-->Manager: AssetsTradeSucessProcess: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);


	
	char sCoin1[COINNAMELEN]={0} ;
	char sCoin2[COINNAMELEN]={0} ;
	double fAmount , fTradeFee;
	m_pDb->StartTransaction();
	
	const char *pStr = strstr(pSymbol,"/");   
	if(NULL == pStr)
	{
		fAmount = fDealNum;
		XERROR("C2C-->ManagerSrv: AssetsTradeSucessProcess,json:[%s] \n ",pData->sConsumerBuf);
		
		// 买家 
		snprintf(m_sSQL,SQLLEN - 1 , "UPDATE user_assets SET  Total=Total+%lf, Available=Available + %lf \
		WHERE userid=%ld and CoinID=(select id from Coin where CoinName = '%s') ;",
		fAmount,fAmount,nBidUserID,pSymbol);
		if(-1 == UpdateSingleRow(m_sSQL,"AssetsTradeSucessProcess"))
		{
			m_pDb->RollBack();
			return -1;
		}

		// 卖家
		snprintf(m_sSQL,SQLLEN - 1 , "UPDATE user_assets SET  Total=Total-%lf, frozen=frozen - %lf \
		WHERE userid=%ld and CoinID=(select id from Coin where CoinName = '%s') ;",
		fAmount,fAmount,nAskUserID,pSymbol);
		if(-1 == UpdateSingleRow(m_sSQL,"AssetsTradeSucessProcess"))
		{
			m_pDb->RollBack();
			return -1;
		}
	
		XINFO("C2C-->ManagerSrv,AssetsTradeSucessProcess,Sucess,json:[%s]\n",pData->sConsumerBuf);
		m_pDb->Commint();
		
		return 0;
	}
	
	XINFO("OrderSrv-->ManagerSrv:AssetsTradeSucessProcess, json:[%s] \n ",pData->sConsumerBuf);

	fAmount = fDealNum * fDealPrice ;
	if( (pStr-pSymbol >COINNAMELEN -1) || (strlen(pStr+1) > COINNAMELEN -1)  )
	{
		XERROR("OrderSrv-->ManagerSrv: AssetsTradeSucessProcess: CoinName is too long , json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	strncpy(sCoin1,pSymbol,pStr-pSymbol );	
	strncpy(sCoin2,pStr+1, strlen(pStr+1));
	

	// 卖家 ETH/BTC 
	
	// ETH 减 
	snprintf(m_sSQL,SQLLEN - 1 , "UPDATE user_assets SET  Total=Total-%lf, frozen=frozen - %lf \
		WHERE userid=%ld and CoinID=(select id from Coin where CoinName = '%s') ;",
		fDealNum,fDealNum,nAskUserID,sCoin1);
	if(-1 == UpdateSingleRow(m_sSQL,"AssetsTradeSucessProcess"))
	{
		m_pDb->RollBack();
		return -1;
	}
	
	// BTC 加
	fAmount  = GetTradeFee(sCoin2);
	snprintf(m_sSQL,SQLLEN - 1 , "UPDATE user_assets SET  Total=Total+%lf-%lf, Available=Available + %lf-%lf \
		WHERE userid=%ld and CoinID=(select id from Coin where CoinName = '%s') ;",
		fAmount,fAmount*fTradeFee,fAmount,fAmount*fTradeFee,nAskUserID,sCoin2);
	if(-1 == UpdateSingleRow(m_sSQL,"AssetsTradeSucessProcess"))
	{
		m_pDb->RollBack();
		return -1;
	}
	

	// 买家  ETH/BTC   
	//   ETH加
	fAmount  = GetTradeFee(sCoin1);
	snprintf(m_sSQL,SQLLEN - 1 , "UPDATE user_assets SET  Total=Total+%lf-%lf, Available=Available + %lf-%lf \
		WHERE userid=%ld and CoinID=(select id from Coin where CoinName = '%s') ;",
		fDealNum,fDealNum*fTradeFee,fDealNum,fDealNum*fTradeFee,nBidUserID,sCoin1);
	if(-1 == UpdateSingleRow(m_sSQL,"AssetsTradeSucessProcess"))
	{
		m_pDb->RollBack();
		return -1;
	}
	
	// BTC 减
	snprintf(m_sSQL,SQLLEN - 1 , "UPDATE user_assets SET  Total=Total-%lf,frozen=frozen - %lf \
		WHERE userid=%ld and CoinID=(select id from Coin where CoinName = '%s') ;",
		fAmount,fAmount,nBidUserID,sCoin2);
		
	if(-1 == UpdateSingleRow(m_sSQL,"AssetsTradeSucessProcess"))
	{
		m_pDb->RollBack();
		return -1;
	}
	
	XINFO("OrderSrv-->ManagerSrv,AssetsTradeSucessProcess,Sucess,json:[%s]\n",pData->sConsumerBuf);
	m_pDb->Commint();
	return 0;

}


int CAssetsMgr::UserWithDrawProcess(STConsumerData *pData)
{
	
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("UserWithDrawProcess Freeze/UnfreezeProcess json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	
	rapidjson::Value::ConstMemberIterator it;

	
	if(((it = doc.FindMember("SessionID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("UserWithDrawProcess: Get SessionID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSessionId = it->value.GetString() ; 

	if(((it = doc.FindMember("RequestID")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("UserWithDrawProcess: Get RequestID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pRequestID = it->value.GetString() ; 
	
	if(((it = doc.FindMember("WSID")) == doc.MemberEnd()) || !it->value.IsInt()) 
	{
		XERROR("UserWithDrawProcess: Get WSID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nWsId = it->value.GetInt(); 

	if(( (it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64())
	{
		XERROR("UserWithDrawProcess :Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId = it->value.GetInt64();

	if(( (it = doc.FindMember("Amount")) == doc.MemberEnd()) )
	{
		XERROR("UserWithDrawProcess :Amount is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	
	double fAmount ;
	if (it->value.IsDouble())
	{
	 fAmount = it->value.GetDouble();
	} 
	else if (it->value.IsInt())
	{
		fAmount = it->value.GetInt();
	}
	else
	{
		XERROR("UserWithDrawProcess :Amount is not int/double, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	

	if(((it = doc.FindMember("CoinName")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("UserWithDrawProcess: Get CoinName is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pCoinName = it->value.GetString() ; 


	if(((it = doc.FindMember("TradePwd")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("UserWithDrawProcess: Get TradePwd is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pTradePwd = it->value.GetString() ; 


	if(((it = doc.FindMember("DestAddr")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("UserWithDrawProcess: Get DestAddr is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pDestAddr = it->value.GetString() ; 

	
	XINFO("UserWithDrawProcess: Recv json:[%s]\n",pData->sConsumerBuf);

	if(((it = doc.FindMember("SmsCode")) == doc.MemberEnd()) || !it->value.IsString()) 
	{
		XERROR("UserWithDrawProcess: Get SmsCode is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSmsCode = it->value.GetString() ; 

	XINFO("WS-->Manager: UserWithDrawProcess: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);
	//1 密码验证
	char m_sSQL[SQLLEN]={0};
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL,SQLLEN,"Select MobilePhoneNum from User where ID = %ld and TradePassword='%s';",nUserId,pTradePwd);		
	
	MySqlRecordSet rs ;
	size_t rows  = GetDataListFromDB(rs,m_sSQL,"UserWithDrawProcess");
	if(rows != 1)
	{
		XERROR("UserWithDrawProcess: ,Get mobilephone fail by userid and tradepwd,  rows:[%d],SQL:[%ld] \n ",rows,m_sSQL );
		SendFailMsg(TAG_USERWITHDRAW_RESP ,pSessionId, nWsId,pData->sConsumerBuf);
		return -1;
	}
	const char *pMobilePhone = rs.GetFieldByID(0, 0).c_str();
	

	//2 手机验证码检查
	int status = CheckCode(pMobilePhone,pSmsCode,pData->sConsumerBuf );
	if (-1 == status)
	{
		SendFailMsg(TAG_USERWITHDRAW_RESP ,pSessionId, nWsId,pData->sConsumerBuf);
		return -1;
	}


	
	// 3. 如果是ETH,那么还要冻结资金池
	
	memset(m_sSQL,0,SQLLEN);
    //snprintf(m_sSQL,SQLLEN-1,"select ChainType from BLOCKCHAIN_INFO where id = ( select BlockChainID from Coin where CoinName = '%s') ;", pCoinName );//savin
    snprintf(m_sSQL,SQLLEN-1,"select BlockChainID from Coin where CoinName = '%s';", pCoinName );
	rows  = GetDataListFromDB(rs,m_sSQL,"Query coin is Ethereum blockchain? ");
	if(rows != 1)
	{
		SendFailMsg(TAG_USERWITHDRAW_RESP ,pSessionId, nWsId,pData->sConsumerBuf);
		return -1;
	}

	m_pDb->StartTransaction();
	int nChainType =  atol(rs.GetFieldByID(0, 0).c_str()) ;
	if(ETH_BLOCKCHAIN == nChainType)
	{
		
		//冻结pool资产
		
		memset(m_sSQL,0,SQLLEN);
		snprintf(m_sSQL,SQLLEN-1,"update EXCHANGE_ASSERTSPOOL set Available=Available-%lf frozen=frozen+%lf   \
			WHERE Available>%lf and  CoinId=(select id from Coin where CoinName ='%s');",
				fAmount,fAmount,fAmount,pCoinName);

		if (-1 == (UpdateSingleRow(m_sSQL,"UserWithDrawProcess: Ethereum blockchain coin  withdraw to useraddr from pool ")))
		{
			m_pDb->RollBack();
			SendFailMsg(TAG_USERWITHDRAW_RESP ,pSessionId, nWsId,pData->sConsumerBuf);
			return -1;
		}

		memset(m_sSQL,0,SQLLEN);
		snprintf(m_sSQL,SQLLEN-1,"select Address, PrivateKey from EXCHANGE_ASSERTSPOOL \
			where CoinId = (select id from Coin where CoinName=%s)" ,
			pCoinName );
	}
	else
	{
		memset(m_sSQL,0,SQLLEN);
        snprintf(m_sSQL,SQLLEN-1,"select DeAdress, PrivateKey from user_depositaddress \
            where UserID =%ld and BlockChainID = (select BlockChainID from Coin where CoinName='%s') ;" ,
            nUserId,pCoinName );//savin
//        snprintf(m_sSQL,SQLLEN-1,"select DeAdress, PrivateKey from user_withdrawaddress \
//			where UserID =%ld and BlockChainID = (select BlockChainID from Coin where CoinName='%s') ;" ,
//			nUserId,pCoinName );//sam
	}


	//4. 获取 SrcAddr, key
	MySqlRecordSet rs2 ;
	rows  = GetDataListFromDB(rs2,m_sSQL,"UserWithDrawProcess Get Src,key ");
	if(rows != 1)
	{
		m_pDb->RollBack();
		SendFailMsg(TAG_USERWITHDRAW_RESP ,pSessionId, nWsId,pData->sConsumerBuf);
		return -1;
	}
	char const *pSrcAddr =  rs2.GetFieldByID(0, 0).c_str();
	char const *pPriKey =  rs2.GetFieldByID(0, 1).c_str();
	


	//5. 冻结用户资金
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL,SQLLEN - 1 , "UPDATE user_assets SET available=Available-%lf,frozen=frozen+%lf \
			WHERE Available>%lf and userid=%ld and CoinID=(select id from Coin where CoinName = '%s') ;",
			fAmount,fAmount,fAmount,nUserId,pCoinName);
			
	if(-1  == UpdateSingleRow(m_sSQL,"UserWithDrawProcess FREEZE"))
	{
		m_pDb->RollBack();
		SendFailMsg(TAG_USERWITHDRAW_RESP ,pSessionId, nWsId,pData->sConsumerBuf);
		return -1;
	}
	
	

	//6.  插入资金申请表
	memset(m_sSQL,0,SQLLEN);
	snprintf(m_sSQL,SQLLEN-1,"insert into WITHDRAW_APPLY(UserID,CoinID,Type,Amount,SrcAddress,DestAddress,PrivateKey,ApplyTime,IsCSConfirm) \
		value(%ld, (select id from coin where CoinName='%s') ,%d,%lf,'%s','%s','%s',sysdate() ,0) ;" ,
		nUserId , pCoinName , TAG_DEPOSIT_POOL2USER,fAmount ,pSrcAddr ,pDestAddr, pPriKey ) ;
	if(-1  == UpdateSingleRow(m_sSQL,"UserWithDrawProcess "))
	{
		m_pDb->RollBack();
		SendFailMsg(TAG_USERWITHDRAW_RESP ,pSessionId, nWsId,pData->sConsumerBuf);
		return -1;
	}

	XINFO("UserWithDrawProcess: Sucess,json:[%s]\n",pData->sConsumerBuf );
	m_pDb->Commint();


	memset(m_sJson,0,JSONLEN);
	snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"SessionID\":\"%s\",\"Status\":\"%d\"}",TAG_USERWITHDRAW_RESP,pSessionId,status );
	char sRoutingKey[ROUTINGKEYLEN]={0};
	snprintf(sRoutingKey,ROUTINGKEYLEN-1,"%s.%d",RTKEY_TOWEBSRV,nWsId);
	
	m_pTnode->PublishToMQ(sRoutingKey, m_sJson,strlen(m_sJson));
	XINFO("ManagerSrv-->WS,UserWithDrawProcess: Sucess,routingkey[%s],json:[%s],Tag[0x%x]\n",sRoutingKey,m_sJson ,pData->nTag);
	
	return 0;
	
}

int CAssetsMgr::AssetsDepositProcess(STConsumerData *pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("AssetsDepositProcess Freeze/UnfreezeProcess json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	
	rapidjson::Value::ConstMemberIterator it;

    if(( (it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64())
	{
		XERROR("AssetsDepositProcess :Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId = it->value.GetInt64();

	if(( (it = doc.FindMember("CoinID")) == doc.MemberEnd()) || !it->value.IsInt64())
	{
		XERROR("AssetsDepositProcess :Get CoinID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
    //long nCoinId = it->value.IsInt64();
    long nCoinId = it->value.GetInt64();

	if(( (it = doc.FindMember("DestAddr")) == doc.MemberEnd()) || !it->value.IsString())
	{
		XERROR("AssetsDepositProcess :DestAddrUserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pDestAddr = it->value.GetString();

	if(( (it = doc.FindMember("Amount")) == doc.MemberEnd()) || !it->value.IsDouble())
	{
		XERROR("AssetsDepositProcess :Amount is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	double fAmount = it->value.GetDouble();

    if(( (it = doc.FindMember("Netfee")) == doc.MemberEnd()) || !it->value.IsDouble())
    {
        XERROR("AssetsDepositProcess :Amount is error, json:[%s] \n ",pData->sConsumerBuf);
        return -1 ;
    }
    double netfee = it->value.GetDouble();


    if(( (it = doc.FindMember("Usedfee")) == doc.MemberEnd()) || !it->value.IsDouble())
    {
        XERROR("AssetsDepositProcess :Amount is error, json:[%s] \n ",pData->sConsumerBuf);
        return -1 ;
    }
    double usedfee = it->value.GetDouble();


	XINFO("WS-->Manager: AssetsDepositProcess: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);

	memset(m_sSQL,0,sizeof(m_sSQL));
	
	if(pData->nTag == TAG_DEPOSIT_USER2EXCHANGE)
	{
		snprintf(m_sSQL,SQLLEN-1,"update user_assets set Total=Total+%lf,Available=Available+%lf where UserID=%ld and CoinID=%ld ;",
			fAmount,fAmount,nUserId,nCoinId);
		if(-1 == UpdateSingleRow(m_sSQL,"User deposit-->exchangeaddr"))
		{
			return -1;
		}
		XINFO("User deposit-->exchangeaddr Sucess,json:[%s] \n ",pData->sConsumerBuf);
    }
	else if(pData->nTag == TAG_DEPOSIT_EXCHANGE2POOL)
	{
        snprintf(m_sSQL,SQLLEN-1,"update EXCHANGE_ASSETSPOOL set Total=Total+%lf-%lf,Available=Available+%lf-%lf where CoinID=%ld ;",
            fAmount,usedfee, fAmount,usedfee, nCoinId);
		if(-1 == UpdateSingleRow(m_sSQL,"ExchangeAddr-->poolAddr"))
		{
			return -1;
		}
		XINFO("ExchangeAddr-->poolAddr Sucess,json:[%s] \n ",pData->sConsumerBuf);
	}
	else if(pData->nTag == TAG_DEPOSIT_POOL2USER) // 用户出金成功
	{
		m_pDb->StartTransaction();
		//1. 用户账户扣钱
		snprintf(m_sSQL,SQLLEN-1,"update user_assets set Total=Total-%lf, frozen=frozen-%lf where UserID=%ld and CoinID=%ld ;",
			fAmount,fAmount,nUserId,nCoinId);
		if(-1 == UpdateSingleRow(m_sSQL,"User withdraw to self addr"))
		{
			m_pDb->RollBack();
			return -1;
		}

		
		//如果这个币是以太坊类的币,需要从pool里里扣钱
		memset(m_sSQL,0,SQLLEN);
//		snprintf(m_sSQL,SQLLEN-1,"select ChainType from BLOCKCHAIN_INFO wehre id = ( select BlockChainID from Coin where id = %ld) ;",nCoinId);
        snprintf(m_sSQL,SQLLEN-1,"select BlockChainID from Coin where id = %ld;",nCoinId);
		
		MySqlRecordSet rs ;
		size_t rows  = GetDataListFromDB(rs,m_sSQL,"Query coin is Ethereum blockchain? ");
		if(rows == 0)
		{
			m_pDb->Commint();
			return 0;
		}
		int nChainType =  atol(rs.GetFieldByID(0, 0).c_str()) ;
		if(ETH_BLOCKCHAIN == nChainType)
		{
			memset(m_sSQL,0,SQLLEN);
//			snprintf(m_sSQL,SQLLEN-1,"update EXCHANGE_ASSERTSPOOL set Total=Total-%lf,Available=Available-%lf where CoinID=%ld ;",
//				fAmount,fAmount,nCoinId);//savin
            snprintf(m_sSQL,SQLLEN-1,"update EXCHANGE_ASSERTSPOOL set Total=Total-%lf,frozen=frozen-%lf where CoinID=%ld ;",
                fAmount,fAmount,nCoinId);

			if ( -1 == UpdateSingleRow(m_sSQL,"Ethereum blockchain coin  withdraw to useraddr from pool "))
			{
				m_pDb->RollBack();
				return -1;
			}
		}

		XINFO("Deposit Pool-->User,Sucess,json:[%s] \n ",pData->sConsumerBuf);
		m_pDb->Commint();
		
	}
	else
	{
		XERROR("AssetsDepositProcess :Tag is error, json:[%s] \n ",pData->sConsumerBuf);
	}

	return 0;

}



int CAssetsMgr::AssetsFreezeProcess(STConsumerData *pData)
{
	Document doc;
	doc.Parse<0>(pData->sConsumerBuf);
	if (doc.HasParseError())
	{
		XERROR("Assets Freeze/UnfreezeProcess json parse error,data:[%s]",   pData->sConsumerBuf );   
		return -1;
	}
	
	rapidjson::Value::ConstMemberIterator it;

    if(( (it = doc.FindMember("UserID")) == doc.MemberEnd()) || !it->value.IsInt64())
	{
		XERROR("AssetsFreezeProcess:Get UserID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nUserId = it->value.GetInt64();
	
	
	if(( (it = doc.FindMember("Symbol")) == doc.MemberEnd()) || !it->value.IsString())
	{
		XERROR("AssetsFreezeProcess:Get Symbol is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	const char *pSymbol = it->value.GetString();
	

	if(( (it = doc.FindMember("TransType")) == doc.MemberEnd()) || !it->value.IsInt())
	{
		XERROR("AssetsFreezeProcess:Get TransType is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	int nTransType = it->value.GetInt();
	

	if(( (it = doc.FindMember("OrderNumber")) == doc.MemberEnd()) || !it->value.IsDouble())
	{
		XERROR("AssetsFreezeProcess:Get OrderNumber is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	double  fOrderNumber = it->value.GetDouble();
	

	if(( (it = doc.FindMember("Price")) == doc.MemberEnd()) || !it->value.IsDouble())
	{
		XERROR("AssetsFreezeProcess:Get Price is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	double  fPrice = it->value.GetDouble();

	
	double fAmount ;
	
	if(( (it = doc.FindMember("OrderID")) == doc.MemberEnd()) || !it->value.IsInt64())
	{
		XERROR("AssetsFreezeProcess:Get OrderID is error, json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	long nOrderId = it->value.GetInt64();
	
	
	
	int isHasIdentify = -1;
	char sSrvIdentify[128]={0};
	if(( (it = doc.FindMember("SrvIdentify")) == doc.MemberEnd()) || !it->value.IsString())
	{
		XINFO("AssetsFreezeProcess:no SrvIdentify in json:[%s] \n ",pData->sConsumerBuf);
	}
	else
	{
		isHasIdentify = 1;
		strncpy(sSrvIdentify,it->value.GetString(),127);
	}


	XINFO("WS-->Manager: AssetsFreezeProcess: tag:[0x%x],JSON:%s",pData->nTag,pData->sConsumerBuf);
	
	char sCoinName[COINNAMELEN];
	memset(sCoinName,0,COINNAMELEN);

	int isC2C = 0;
	const char *pStr = strstr(pSymbol,"/");   
	
	if(NULL == pStr)
	{
		fAmount = fOrderNumber ;
		XINFO("AssetsFreezeProcess: C2C-->ManagerSrv ,json:[%s] \n ",pData->sConsumerBuf);
		strncpy(sCoinName,pSymbol,COINNAMELEN-1);
	}
	else
	{
		XINFO("AssetsFreezeProcess: OrderSrv-->ManagerSrv ,json:[%s] \n ",pData->sConsumerBuf);

		
		//if(0 == nTransType) //交易类型，0 表示买， 1表示卖
		if(BUY_ORDER == nTransType) //交易类型，1 表示买， 2 表示卖
		{
			fAmount = fOrderNumber * fPrice;
			strncpy(sCoinName,pStr+1,COINNAMELEN-1);
			
		}
		else if(SELL_ORDER == nTransType)
		{
			fAmount = fOrderNumber ;
			if(pStr-pSymbol < (COINNAMELEN-1))
			{
				strncpy(sCoinName,pSymbol,pStr-pSymbol);
				
			}
			else
			{
				XERROR("AssetsFreezeProcess: first coinname  is too long in json:[%s] \n ",pData->sConsumerBuf);
	        	return -1 ;
			}
		}
		else
		{
			XERROR("AssetsFreezeProcess: TransType value is error in json:[%s] \n ",pData->sConsumerBuf);
			return -1 ;
		}
	}


	memset(m_sSQL,0,SQLLEN);
	
	if(TAG_FREEZE == pData->nTag) // 冻结
	{
		if(1 == isHasIdentify)
		{
			snprintf(m_sSQL,SQLLEN-1,"UPDATE user_assets SET available=Available-%lf,frozen=frozen+%lf \
			WHERE Available>%lf and userid=%ld and CoinID=(select id from Coin where CoinName = '%s') ;",
			fAmount,fAmount,fAmount,nUserId,sCoinName);
			
			isHasIdentify = UpdateSingleRow(m_sSQL,"FREEZE");
		}

		char sRespone[CONSUMERBUFLEN];
		memset(m_sJson,0,JSONLEN);
		snprintf(m_sJson,JSONLEN-1,"{\"Tag\":%d,\"UserID\":%ld,\"OrderID\":%ld,\"Status\":%d}",
				TAG_FREEZE_RESP,nUserId,nOrderId,isHasIdentify);
		m_pTnode->PublishToMQ( sSrvIdentify, m_sJson,strlen(m_sJson));

		XINFO("ManagerSrv-->C2C/OrderSrv, freesz ,routingkey:[%s], freesz json:[%s],Tag[0x%x]\n",sSrvIdentify,m_sJson,pData->nTag);
		
	}
	else if(TAG_UNFREEZE == pData->nTag) // 解冻
	{
		snprintf(m_sSQL,SQLLEN-1,"UPDATE user_assets SET available=Available+%lf  ,frozen=frozen-%lf \
		WHERE userid=%ld and CoinID=(select id from Coin where CoinName = '%s') ;",
		fAmount,fAmount,nUserId,sCoinName);
		
		if(0 == UpdateSingleRow(m_sSQL,"C2C/OrderSrv-->ManagerSrv unfreeze ") )
		{

			XINFO(" unfreesz json:[%s]\n",pData->sConsumerBuf);
		}
	}
	else
	{
		XERROR("AssetsFreezeProcess: tag value is error in json:[%s] \n ",pData->sConsumerBuf);
		return -1 ;
	}
	
	

	return 0;
}



void *AssetsProcess(void *arg)
{
	CAssetsMgr assetsMgr;
	if(assetsMgr.Init())return NULL;
	printf("AssetsProcess init sucess \n ");	

	while( 0 == assetsMgr.GetError())
	{
		pthread_mutex_lock(&_AssetsReadLock);
		STConsumerData *pData = (STConsumerData *)g_pAssetsDataPool->GetDataNode();
		pthread_mutex_unlock(&_AssetsReadLock);
		if(NULL == pData)
		{
			usleep(5000);
			continue;
		}
		
		switch(pData->nTag)
		{
			case TAG_FREEZE:  					// 冻结请求: 订单挂单请求,出金请求
			case TAG_UNFREEZE:					// 解冻结请求: 取消订单,出金请求审核失败
				assetsMgr.AssetsFreezeProcess(pData);
				break;
			case TAG_DEPOSIT_POOL2USER:			// 入金通知 
			case TAG_DEPOSIT_USER2EXCHANGE:
			case TAG_DEPOSIT_EXCHANGE2POOL:
				assetsMgr.AssetsDepositProcess(pData);
				break;
			case TAG_USERWITHDRAW:
				assetsMgr.UserWithDrawProcess(pData);
				break;
			case TAG_TRADESUCESS:				// 交易成功	
				assetsMgr.AssetsTradeSucessProcess(pData);
				break;
			default:
				XERROR("AssetsModule: don't support tag:0x%x, data:[%s]\n",pData->nTag, pData->sConsumerBuf);
		}

        //g_pAssetsDataPool->PushUsedNode(pData);
        g_pMemNodePool->PushUsedNode(pData);

	}
	
	XERROR("AssetsProcess Exit !");
	
	return NULL;
}


int AssetsModule()
{
	pthread_mutex_init(&_AssetsReadLock,NULL);
	
	for(int i=0;i<g_stConfig.nAssetsModuleNum;i++)
	{
		pthread_attr_t attr;
		pthread_attr_init (&attr);   
		pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);   
		pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);  
		
		pthread_t nThreadID;
		
		if (0 != pthread_create(&nThreadID, &attr, AssetsProcess, NULL) )
		{
			XERROR("pthread_create AssetsModule error:%s\n",strerror(errno));
			return -1;
		}
	}
	
	return 0;
}
