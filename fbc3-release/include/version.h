#ifndef __VERSION_H__
#define __VERSION_H__

#include "version_autogenarated.h"

extern void print_build_version_info ( void );
extern const char *fbc_get_version_info();
extern const char *fbc_get_git_version_info();
extern const char *fbc_get_last_chaned_time_info();
extern const char *fbc_get_git_branch_info();
extern const char *fbc_get_build_time_info();
extern const char *fbc_get_build_name_info();

#endif