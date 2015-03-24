#include "SockHelper.h"

int main()
{
	UDPHelper h(9003);
	char buf[1024];
	while(1)
	{
		h.readn(buf, 1024);
		cout <<buf<<endl;
	}
	return 0;
}
