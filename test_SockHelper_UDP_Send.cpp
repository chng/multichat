#include "SockHelper.h"

int main()
{
	UDPHelper h("127.0.0.1", 9003);
	int count = 9;
	while(count--)
	{
		h.writen("hello UDP\n", 11);
	}
	return 0;
}
