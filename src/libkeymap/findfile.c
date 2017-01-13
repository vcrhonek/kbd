#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "nls.h"
#include "findfileP.h"
#include "keymap/findfile.h"

void lk_fpclose(lkfile_t *fp)
{
	fpclose(fp);
}

int lk_findfile(const char *fnam, const char *const *dirpath, const char *const *suffixes, lkfile_t *fp)
{
	return findfile(fnam, dirpath, suffixes, fp);
}

lkfile_t *lk_fpopen(const char *filename)
{
	return fpopen(filename);
}
