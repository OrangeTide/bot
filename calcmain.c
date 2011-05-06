#include <stdio.h>
#include "xcalc.h"

int main(int argc, char **argv)
{
	int i;
	struct num *num;

	for (i = 1; i < argc; i++) {
		num = xcalc(argv[i]);
		if (num)
			printf("%s <- %s\n", num_string(num), argv[i]);
		else
			printf("ERR!\n");
		num_free(num);
	}
	return 0;
}
