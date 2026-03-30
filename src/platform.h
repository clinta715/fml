#ifndef PLATFORM_H
#define PLATFORM_H

#include "fml.h"

typedef struct {
    char name[MAX_NAME];
    EntryType type;
    uint64_t size;
    time_t mtime;
} PlatformDirEntry;

typedef int (*ProgressCallback)(uint64_t done, uint64_t total, const char *filename, void *userdata);

int pl_init(void);
void pl_cleanup(void);

char *pl_get_cwd(void);
int pl_set_cwd(const char *path);

char *pl_home_dir(void);
char *pl_path_join(const char *a, const char *b);

int pl_list_dir(const char *path, PlatformDirEntry **entries, int *count);
int pl_is_dir(const char *path);
int pl_file_exists(const char *path);
uint64_t pl_file_size(const char *path);
time_t pl_file_mtime(const char *path);

int pl_copy_file(const char *src, const char *dst);
int pl_copy_dir(const char *src, const char *dst);
int pl_copy_file_progress(const char *src, const char *dst, ProgressCallback cb, void *userdata);
int pl_copy_dir_progress(const char *src, const char *dst, ProgressCallback cb, void *userdata);
int pl_move(const char *src, const char *dst);
int pl_delete(const char *path);
int pl_mkdir(const char *path);

int pl_is_text_file(const char *path);
char *pl_read_file(const char *path, size_t max_bytes, size_t *out_len);

#endif