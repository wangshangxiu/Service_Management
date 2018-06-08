
#include "Public.h"


int main(int argc, char *argv[])
{
	if( -1 == Init())return -1;

	if(-1 == AssetsModule())return -1;
	
	if(-1 == LoginOutModule())return -1;
	
	if(-1 == CreateManagerModule())return -1;
	
	CheckExit();
	
	return 0;
}
