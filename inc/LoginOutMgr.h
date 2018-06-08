/*******************************************************************************************

*Department:	R&D Exchange 

*Decrible:  	ManagerSrv Login/Logout Mgr module
				
				


*Auhor:			Savin

*Createdtime:	2018-03-13

*ModifyTime:	


********************************************************************************************/


#ifndef EXCHANGE_LOGINOUT_HEAD_H
#define EXCHANGE_LOGINOUT_HEAD_H

#include "MgrBase.h"

class CLoginOutMgr : public CMgrBase
{
public:

	int LoginOut(STConsumerData *pData);
};




#endif //EXCHANGE_LOGINOUT_HEAD_H

