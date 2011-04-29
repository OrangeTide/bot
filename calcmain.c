#include <stdio.h>
#include "xcalc.h"

int main(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++) {
		long long v;
		if (xcalc(argv[i], &v))
			printf("ERR!\n");
		else
			printf("%llu <- %s\n", v, argv[i]);
	}
	return 0;
}
