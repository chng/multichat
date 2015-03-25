#include <iostream>
#include "singleton.h"
using namespace std;

struct A
{
	int a = 1;
};

singleton<A> singleton_a;

int main()
{
	A *p = singleton<A>::getInstance();
	cout <<p<<endl;
	return 0;
}
