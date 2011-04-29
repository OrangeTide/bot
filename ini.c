/* This software is public domain. No copyright is claimed.
 * Jon Mayo April 2011
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Load an .ini format file
 * filename - path to a file
 * report - callback can return non-zero to stop, the callback error code is
 *     returned from this function.
 * return - return 0 on success
 */
int ini_load(const char *filename,
	int (*report)(const char *section, const char *name, const char *value))
{
	char name[64];
	char value[256];
	char section[128] = "";
	char *s;
	FILE *f;
	int cnt;
	int ret;
	int line;

	f = fopen(filename, "r");
	if (!f) {
		perror(filename);
		return -1;
	}

	ret = 0;
	line = 0;
	while (!feof(f)) {
		line++;
		if (fscanf(f, "[%127[^];\n]]", section) == 1) {
		} else if ((cnt = fscanf(f, " %63[^=;\n] = %255[^;\n]",
			name, value))) {
			if (cnt == 1)
				*value = 0;
			for (s = name + strlen(name) - 1; s > name &&
				isspace(*s); s--)
				*s = 0;
			for (s = value + strlen(value) - 1; s > value &&
				isspace(*s); s--)
				*s = 0;
			ret = report(section, name, value);
			if (ret)
				goto out;
		}
		fscanf(f, " ;%*[^\n]");
	}

out:
	if (ret) {
		fprintf(stderr, "%s:error on line %d\n", filename, line);
	}
	fclose(f);
	return ret;
}

/* test code */
#if 0
static int my_callback(const char *section, const char *name, const char *value)
{
	fprintf(stdout, "[%s] '%s'='%s'\n", section, name, value);
	return 0;
}

int main(int argc, char **argv)
{
	int i, e;
	for(i = 1; i < argc; i++) {
		e = ini_load(argv[i], my_callback);
		if (e)
			return 1;
	}
	return 0;
}
#endif
