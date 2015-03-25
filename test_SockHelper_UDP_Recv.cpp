#include "SockHelper.h"

int main()
{
	UDPHelper h(9001);
	char buf[1024];
	while(true)
	{
		h.readn(buf, 1024);
		cout <<buf<<endl;
		SA_IN addr = h.getRemoteAddr();
		cout <<"address "<<addr.sin_addr.s_addr<<endl;
		cout <<"port "<<addr.sin_port<<endl;
	}
	return 0;
}
