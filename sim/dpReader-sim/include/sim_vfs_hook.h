#pragma once
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include "sim_vfs.h"

#ifndef SIM_VFS_NO_HOOK
#define fopen(p, m) sim_fopen(p, m)
#define open(p, ...) sim_open(p, __VA_ARGS__)
#define opendir(p) sim_opendir(p)
#define stat(p, b) sim_stat(p, b)
#define access(p, m) sim_access(p, m)
#define remove(p) sim_remove(p)
#define unlink(p) sim_unlink(p)
#define mkdir(p, m) sim_mkdir(p, m)
#define rename(o, n) sim_rename(o, n)
#define rmdir(p) sim_rmdir(p)
#endif
