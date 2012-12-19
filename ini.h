/* This software is public domain. No copyright is claimed.
 * Jon Mayo April 2011
 */
#ifndef INI_H
#define INI_H
int ini_load(const char *filename,
	int (*report)(const char *section,const char *name,const char *value ));
#endif
