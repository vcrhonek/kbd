#ifndef FINDFILEP_H
#define FINDFILEP_H

#include <stdio.h>
#include <sys/param.h>

#include <keymap/findfile.h>

typedef lkfile_t fpfile_t;

fpfile_t *fpopen(const char *filename);
void fpclose(fpfile_t *fp);
int findfile(const char *fnam, const char *const *dirpath, const char *const *suffixes, fpfile_t *fp);

#endif /* FINDFILEP_H */
