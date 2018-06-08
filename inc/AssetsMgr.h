/*******************************************************************************************

*Department:	R&D Exchange 

*Decrible:  	ManagerSrv Assets Mgr module
				
				


*Auhor:			Savin

*Createdtime:	2018-03-13

*ModifyTime:	


********************************************************************************************/


#ifndef EXCHANGE_ASSETSMGR_HEAD_H
#define EXCHANGE_ASSETSMGR_HEAD_H

#include "MgrBase.h"

class CAssetsMgr : public CMgrBase
{
public:

	int AssetsFreezeProcess(STConsumerData *pData);
	int AssetsDepositProcess(STConsumerData *pData);
	int UserWithDrawProcess(STConsumerData *pData);
	int AssetsTradeSucessProcess(STConsumerData *pData);
	int SendFailMsg(int nTag,const char *pSessionId,int nWsId, const char *pJson);

};




#endif //EXCHANGE_ASSETSMGR_HEAD_H

