/*******************************************************************************************

*Department:	TP

*Decrible:  	FIX SiganlEngine
				sudo apt-get install libmysqlclient-dev
				

*Auhor:			Savin

*Createdtime:	2017-06-07

*ModifyTime:	


********************************************************************************************/


#ifndef EXCHANGE_PUBLIC_HEAD_H
#define EXCHANGE_PUBLIC_HEAD_H

#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <list>
#include <set>


#include "xlogger.h"
#include "mysqlop.h"
#include "redisop.h"
#include "ConfigFileReader.h"

#include "MgrConsumer.h"
#include "MemPoolMgr.h"
#include "tnode.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"



#define SERVICECONFIG "./ManagerSrv.cfg"


using namespace rapidjson;
using rapidjson::Document;
using rapidjson::Value;
using namespace std;
using namespace snetwork_xservice_xflagger;


#define BTC_BLOCKCHAIN 1
#define ETH_BLOCKCHAIN 2	

//timeout
#define SMSCODETIMEOUT 300

//1:买单,2: 卖单
#define BUY_ORDER 1
#define SELL_ORDER 2

//len
#define MEMNODENUMBER 100000
#define CONSUMERBUFLEN 512
#define SQLLEN 1024
#define JSONLEN 1024
#define USERNAMELEN 128
#define RANDOMCODELEN 8
#define ROUTINGKEYLEN 128
#define	EMAILLEN 128
#define COINNAMELEN 32
#define ADDRESSLEN 128
#define CHAINPRIKEY 128
#define BANKACCOUNTLIST 1024*10



//routingkey
#define RTKEY_TOWEBSRV "WebSrv"

#define CONSUMERQUEUE "ManagerQueue"

//tag

#define TAG_REGISTER 0x1001
#define TAG_REGISTER_RESP 0x10011

#define TAG_FREEZE 0x3003
#define TAG_FREEZE_RESP 0X3004
#define TAG_UNFREEZE 0x4005

#define TAG_DEPOSIT_POOL2USER  0x9001
#define TAG_DEPOSIT_USER2EXCHANGE 0x9002
#define TAG_DEPOSIT_EXCHANGE2POOL 0x9003
#define TAG_TRADESUCESS 0x3009

#define TAG_LOGIN 0x2001 
#define TAG_LONIN_RESP 0x2002
#define TAG_LOGOUT 0x20011 
#define TAG_LOGIN_KICK 0x20021

#define TAG_SMSCODEREQ  0x11000
#define TAG_SMSCODEREQ_MODIFYPHONE  0x11002
#define	TAG_SENDSMSCODE 0x11003

#define	TAG_MODIFYLOGINPWD 0x12000
#define	TAG_MODIFYLOGINPWD_RESP 0x12001

#define	TAG_MODIFYTRADEPWD 0x13000
#define	TAG_MODIFYTRADEPWD_RESP 0x13001

#define	TAG_MODIFYMOBILEPHONE 0x14000
#define	TAG_MODIFYMOBILEPHONE_RESP 0x14001

#define TAG_DEPOSITADDR 0x15000
#define TAG_DEPOSITADDR_RESP 0x15001

#define TAG_ADDWITHDRWADDR 0x16000
#define TAG_ADDWITHDRWADDR_RESP 0x16001

#define TAG_DELWITHDRWADDR 0x17000
#define TAG_DELWITHDRWADDR_RESP 0x17001

#define TAG_GETBANKACCOUNTLIST 0x18000
#define TAG_GETBANKACCOUNTLIST_RESP 0x18001

#define TAG_ADDBANKACCOUNT 0x19000
#define TAG_ADDBANKACCOUNT_RESP 0x19001

#define TAG_DELBANKACCOUNT 0x20000
#define TAG_DELBANKACCOUNT_RESP 0x20001

#define TAG_USERWITHDRAW 0x21000
#define TAG_USERWITHDRAW_RESP 0x21001

#define TAG_KYCCommint 0x22000
#define TAG_KYCCommint_RESP 0x22001

#define TAG_GETBANKLIST 0x23000
#define TAG_GETBANKLIST_RESP 0x23001

#define TAG_FORGETLOGINPWD 0x24000
#define TAG_FORGETLOGINPWD_RESP 0x24001 

#define TAG_GETWITHDRAWADDR 0x25000
#define TAG_GETWITHDRAWADDR_RESP 0x25001


#define TAG_QUERYKYC 0x26000
#define TAG_QUERYKYC_RESP 0x26001




typedef struct _STConsumerData
{
	int nTag;
	char sConsumerBuf[CONSUMERBUFLEN];
}STConsumerData;




typedef struct __STConfig
{
	int nCreateAddr_Chkitime;
	int nDeposit_Chkitime;
	//int nConfirm_Chkitime;
	int nTrade_Chkitime;
	string  logdir;
    bool    onlyscreenshowlog;
	
	int nMemNodeNumber;
	int nMemNodeSize;
	int nAssetsModuleNum;

	// MYSQL
	string 	sIp;
	string 	sUser;
	string 	sPwd;
	int  	nPort;
	int  	nMaxconn;
	string	sDBName;
	
	// REDIS 
	string 	sRedisHost;
	int 	nRedisPort ;
	string 	sRedisPwd ;
	int 	nRedisDBIndex;	
	
	// TNODE
	string mq_vhost_token;
	string mq_exchange_token;
	string mq_address_token;
	int    mq_port_token;
	string mq_user_token;
	string mq_password_token;
	string mq_routingkey_token;
	string mq_bindingkey_token;
	string mq_routingkey_tosmssender;
	string mq_queuename;

}STConfig;

extern STConfig g_stConfig;

class CNodePool;

extern CNodePool  *g_pMgrDataPool;
extern CNodePool  *g_pAssetsDataPool;
extern CNodePool  *g_pLoginoutDataPool;
extern CNodePool  *g_pMemNodePool;
extern STConfig    g_stConfig;



int Init();


int InitConsumer() ;


int AssetsModule();

int LoginOutModule();

int CreateManagerModule();

int GenerateRandomCode(char *pRandomCode);



int CheckExit();


#endif //EXCHANGE_PUBLIC_HEAD_H

