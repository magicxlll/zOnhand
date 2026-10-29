#pragma once
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

void sim_vfs_init(const char *storage_root);
const char* sim_translate_path(const char* path, char* buffer, size_t buflen);

FILE* sim_fopen(const char* path, const char* mode);
int sim_open(const char* path, int flags, ...);
DIR* sim_opendir(const char* path);
int sim_stat(const char* path, struct stat* buf);
int sim_access(const char* path, int mode);
int sim_remove(const char* path);
int sim_unlink(const char* path);
int sim_mkdir(const char* path, mode_t mode);
int sim_rename(const char* oldpath, const char* newpath);
int sim_rmdir(const char* path);

#ifdef __cplusplus
}
#endif
