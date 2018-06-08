/*******************************************************************************************

*Department:	R&D Exchange 

*Decrible:  	ManagerSrv Mgr base class
				
				


*Auhor:			Savin

*Createdtime:	2018-03-13

*ModifyTime:	


********************************************************************************************/

#ifndef EXCHANGE_MGRBASE_HEAD_H
#define EXCHANGE_MGRBASE_HEAD_H

#include "Public.h"
#include "tnode.h"

using namespace snetwork_xservice_tnode;

class CRedis;
class MySqlDB;

class CMgrBase
{
public:
	CMgrBase();
	~CMgrBase();
	
	int Init();
	
	int GetError(){return m_nFail;}
	
public:
	int 	GetDataListFromDB(MySqlRecordSet &rs,const char *pSQL, const char *pModuleName);
	int 	UpdateSingleRow(const char *pSQL,const char *pModuleName);
	int 	CheckCode(const char *pKey, const char *pCode, const char *pJson) ;
	double 	GetTradeFee(const char *pCoinName);

	TNode 		*m_pTnode;
	CRedis 		*m_pRedis;
	MySqlDB 	*m_pDb ;
	
	int			m_nFail ;
	
	char m_sSQL[SQLLEN];
	char m_sJson[JSONLEN];
};




#endif //EXCHANGE_MGRBASE_HEAD_H

