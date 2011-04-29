#ifndef INI_H
#define INI_H
int ini_load(const char *filename,
	int (*report)(const char *section,const char *name,const char *value ));
#endif
