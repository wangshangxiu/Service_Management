
#include "Public.h"



class CNodePool;


CNodePool  *g_pMgrDataPool;
CNodePool  *g_pAssetsDataPool;
CNodePool  *g_pLoginoutDataPool;
CNodePool  *g_pMemNodePool;


STConfig g_stConfig;


void InitLogSys()
{
    (*XLogger::GetObject("OnezFixMarket")).StdErr(g_stConfig.onlyscreenshowlog).Colour(true).Dir(g_stConfig.logdir).BuffDelay(0);
    XINFO("InitLogSys ....");
}


int InitMysqlConnPool()
{
	//ConnectionPool*  g_connPool = ConnectionPool::GetInstance(g_stConfig.sIp,g_stConfig.sUser,g_stConfig.sPwd,g_stConfig.nPort,g_stConfig.nMaxconn);

	return 0;
}


int InitPool()
{
	g_pMgrDataPool = new CNodePool();
	if(g_pMgrDataPool == NULL)
	{
		XERROR("InitPool: malloc fail !\n");
		return -1;
	}
	g_pAssetsDataPool = new CNodePool();
	if(g_pAssetsDataPool == NULL)
	{
		XERROR("InitPool: malloc fail !\n");
		return -1;
	}
	
	g_pLoginoutDataPool = new CNodePool();
	if(g_pLoginoutDataPool == NULL)
	{
		XERROR("InitPool: malloc fail !\n");
		return -1;
	}
	
	g_pMemNodePool = new CNodePool();
	if(g_pMemNodePool == NULL)
	{
		XERROR("InitPool: malloc fail !\n");
		return -1;
	}
	
	if( -1 == g_pMemNodePool->Init(g_stConfig.nMemNodeNumber,g_stConfig.nMemNodeSize) )
	{
		XERROR("InitPool: malloc fail !\n");
		return -1;
	}
	
	return 0;
}



int InitDatabase()
{
	ConnectionPool* g_connPool = ConnectionPool::GetInstance(g_stConfig.sIp.c_str(),g_stConfig.sUser.c_str(),
				g_stConfig.sPwd.c_str(),g_stConfig.nPort,g_stConfig.nMaxconn);
	
	if(NULL == g_connPool)return -1;
	
	//g_redisPool = ObjectPool<CRedis, 3>::GetInstance(cfg->RedisHost().c_str(), cfg->RedisPort(), cfg->RedisDB(), cfg->RedisAuth().c_str());

	return 0;
}







int ReadPublicConfig(CConfigFileReader &config)
{
	char *buf = NULL;

    if (NULL == (buf = config.GetConfigName("MemNodeNumber")))
    {
        printf("read Public->MemNodeNumber error \n");
        return -1;
    }
    g_stConfig.nMemNodeNumber = atoi(buf);

	
	if (NULL == (buf = config.GetConfigName("MemNodeSize")))
    {
        printf("read Public->MemNodeSize error \n");
        return -1;
    }
    g_stConfig.nMemNodeSize = atoi(buf);
	
	
	if (NULL == (buf = config.GetConfigName("AssetsModuleNum")))
    {
        printf("read Public->AssetsModuleNum error \n");
        return -1;
    }
    g_stConfig.nAssetsModuleNum = atoi(buf);
		

	if (NULL == (buf = config.GetConfigName("Trade_Chkitime")))
    {
        printf("read RabbitMQInfo->Trade_Chkitime error \n");
        return -1;
    }
    g_stConfig.nTrade_Chkitime = atoi(buf);
	
	if (NULL == (buf = config.GetConfigName("logdir")))
    {
        printf("read RabbitMQInfo->logdir error \n");
        return -1;
    }
    g_stConfig.logdir = buf;

     if (NULL == (buf = config.GetConfigName("onlyscreenshowlog")))
     {
         printf("read onlyscreenshowlog error \n");
         return -1;
     }
     if (1 == atoi(buf))
         g_stConfig.onlyscreenshowlog = true;
     else
         g_stConfig.onlyscreenshowlog = false;


	
	return 0;
}







int ReadMySQLConfig(CConfigFileReader &config)
{
	char *buf = NULL;
	if (NULL == (buf = config.GetConfigName("DB_Ip")))
    {
        printf("read RabbitMQInfo->DB_Ip error \n");
        return -1;
    }
    g_stConfig.sIp = buf;
	
	if (NULL == (buf = config.GetConfigName("DB_User")))
    {
        printf("read RabbitMQInfo->DB_User error \n");
        return -1;
    }
    g_stConfig.sUser = buf;
	
	if (NULL == (buf = config.GetConfigName("DB_Pwd")))
    {
        printf("read RabbitMQInfo->DB_Pwd error \n");
        return -1;
    }
    g_stConfig.sPwd = buf;
	
	if (NULL == (buf = config.GetConfigName("DB_Port")))
    {
        printf("read RabbitMQInfo->DB_Port error \n");
        return -1;
    }
    g_stConfig.nPort = atoi(buf);
	
	if (NULL == (buf = config.GetConfigName("DB_Maxconn")))
    {
        printf("read RabbitMQInfo->DB_Maxconn error \n");
        return -1;
    }
    g_stConfig.nMaxconn = atoi(buf);
	
	if (NULL == (buf = config.GetConfigName("DB_Name")))
    {
        printf("read RabbitMQInfo->DB_Name error \n");
        return -1;
    }
    g_stConfig.sDBName = buf;
	
	return 0;
}







int ReadMQInfoConfig(CConfigFileReader &config)
{
	char *buf = NULL;
	if (NULL == (buf = config.GetConfigName("mq_vhost_token")))
    {
        printf("read RabbitMQInfo->mq_vhost_token error \n");
        return -1;
    }
    g_stConfig.mq_vhost_token = buf;


	if (NULL == (buf = config.GetConfigName("mq_exchange_token")))
    {
        printf("read RabbitMQInfo->mq_exchange_token error \n");
        return -1;
    }
    g_stConfig.mq_exchange_token = buf;

	if (NULL == (buf = config.GetConfigName("mq_address_token")))
    {
        printf("read RabbitMQInfo->mq_address_token error \n");
        return -1;
    }
    g_stConfig.mq_address_token = buf;


	if (NULL == (buf = config.GetConfigName("mq_port_token")))
    {
        printf("read RabbitMQInfo->mq_port_token error \n");
        return -1;
    }
    g_stConfig.mq_port_token = atoi(buf);

	if (NULL == (buf = config.GetConfigName("mq_user_token")))
    {
        printf("read RabbitMQInfo->mq_user_token error \n");
        return -1;
    }
    g_stConfig.mq_user_token = buf;


	if (NULL == (buf = config.GetConfigName("mq_password_token")))
    {
        printf("read RabbitMQInfo->mq_password_token error \n");
        return -1;
    }
    g_stConfig.mq_password_token = buf;

	/*
	if (NULL == (buf = config.GetConfigName("mq_routingkey_token")))
    {
        printf("read RabbitMQInfo->mq_routingkey_token error \n");
        return -1;
    }
    g_stConfig.mq_routingkey_token = buf;	
	*/
	if (NULL == (buf = config.GetConfigName("mq_bindingkey_token")))
    {
        printf("read RabbitMQInfo->mq_bindingkey_token error \n");
        return -1;
    }
    g_stConfig.mq_bindingkey_token = buf;


	if (NULL == (buf = config.GetConfigName("mq_routingkey_tosmssender")))
    {
        printf("read RabbitMQInfo->mq_routingkey_tosmssender error \n");
        return -1;
    }
    g_stConfig.mq_routingkey_tosmssender = buf;	


	if (NULL == (buf = config.GetConfigName("mq_queuename")))
	{
		printf("read RabbitMQInfo->mq_queuename error \n");
		return -1;
	}
	g_stConfig.mq_queuename = buf; 

	return 0;
}



int ReadRedisConfig(CConfigFileReader &config)
{
	char *buf = NULL;
	if (NULL == (buf = config.GetConfigName("RedisHost")))
    {
        printf("read RabbitMQInfo->RedisHost error \n");
        return -1;
    }
    g_stConfig.sRedisHost = buf;
	
	
	if (NULL == (buf = config.GetConfigName("RedisPort")))
    {
        printf("read RabbitMQInfo->RedisPort error \n");
        return -1;
    }
    g_stConfig.nRedisPort = atoi(buf);
		
	if (NULL == (buf = config.GetConfigName("RedisPwd")))
    {
        printf("read RabbitMQInfo->RedisPwd error \n");
        return -1;
    }
    g_stConfig.sRedisPwd = buf;

	if (NULL == (buf = config.GetConfigName("RedisDBIndex")))
    {
        printf("RedisInof->RedisDBIndex error \n");
        return -1;
    }
    g_stConfig.nRedisDBIndex = atoi(buf);
	
	return 0;
}

int ReadConfig()
{
	struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction( SIGPIPE, &sa, 0 );

    CConfigFileReader config;

    if( -1 == config.LoadFile(SERVICECONFIG)  )
    {
        printf("open config file: %s,fail:%s\n",SERVICECONFIG, strerror(errno));
        return -1;
    }

    if(-1==ReadPublicConfig(config))return -1;

    if(-1==ReadMySQLConfig(config))return -1;
	
	if(-1==ReadRedisConfig(config))return -1;
	
	if(-1==ReadMQInfoConfig(config))return -1;

    return 0;

}




int GenerateRandomCode(char *pRandomCode)
{
	int randNum = 0;
    int fd = open("/dev/urandom", O_RDONLY);
    read(fd, (char *)&randNum, sizeof(int));
    close(fd);

    snprintf(pRandomCode,7,"%d",abs(randNum));	

	return 0;
}
 






// 
int CheckExit()
{
	while(1)
	{
		sleep(10);
	}
}



int Init()
{
    if(-1 == ReadConfig())return -1;

    InitLogSys();

    if(-1 == InitDatabase())return -1;

    if(-1 == InitPool())return -1;

	//if(-1 == InitMysqlConnPool())return -1;

	if(-1 == InitConsumer())return -1;

	return 0;
}



