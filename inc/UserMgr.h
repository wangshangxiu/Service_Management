/*******************************************************************************************

*Department:	R&D Exchange 

*Decrible:  	ManagerSrv User Mgr module
				
				


*Auhor:			Savin

*Createdtime:	2018-03-13

*ModifyTime:	


********************************************************************************************/


#ifndef EXCHANGE_USERMGR_HEAD_H
#define EXCHANGE_USERMGR_HEAD_H

#include "MgrBase.h"

class CUserMgr : public CMgrBase
{
public:

	int UserRegister(STConsumerData *pData);
	int SendSMS(STConsumerData *pData);
	int DepositAddrRequest(STConsumerData *pData);
	int AddWithDrawAddr(STConsumerData *pData);
	int DelWithdrawAddr(STConsumerData *pData);
	int AddBankAccount(STConsumerData *pData);
	int DelBankAccount(STConsumerData *pData);
	int KYCResourceCommint(STConsumerData *pData);
	int GetBankList(STConsumerData * pData)	;
	int GetBankAccountList(STConsumerData * pData);
	int GetWithdrawAddrList(STConsumerData * pData);
	int ModifyPhone(STConsumerData *pData);
	int ModifyPassword(STConsumerData *pData);
	int ForgetLoginPwd(STConsumerData * pData);
	int GetKYCResource(STConsumerData * pData);
	
};




#endif //EXCHANGE_USERMGR_HEAD_H

