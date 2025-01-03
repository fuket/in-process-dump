#include "SehException.h"

int run()
{
	printf("ready...\r\n");
	getchar();

	//To output a dump file containing exception information
	__try
	{
		int a = 1;
		int b = 0;
		int c = a / b;
		printf("%d\n", c);
	}
	__except (SehException::GetInstance().Filter(GetExceptionCode(), GetExceptionInformation()))
	{
		printf("in except.\n");
	}
	printf("done.\n");
	getchar();

	return 0;
}

int main()
{
	SehException::GetInstance().SetDumpPath(L".\\dump.dmp");
	return run();
}
