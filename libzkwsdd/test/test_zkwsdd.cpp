#include <stdio.h>
#include <stdlib.h>
#include "../include/zkwsdd.h"
#include <Windows.h>

// ģ�� Target
class MyTarget : public zkwsdd_Target
{
public:
	MyTarget() : zkwsdd_Target("urn:1234-5678-90", "eval", "rtmp://172.16.1.10:4433")
	{
	}
};

int main(int argc, char **argv)
{
	MyTarget target;

	while (1) {
		Sleep(100);
	}

	return 0;
}
