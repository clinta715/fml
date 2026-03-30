#ifndef _WIN32

#include "platform.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>
#include <libgen.h>
#include <strings.h>

static int entry_cmp(const void *a, const void *b) {
    const PlatformDirEntry *ea = a, *eb = b;
    if (ea->type != eb->type) {
        return (ea->type == ENTRY_DIR) ? -1 : 1;
    }
    return strcasecmp(ea->name, eb->name);
}

int pl_init(void) { return 0; }
void pl_cleanup(void) {}

char *pl_get_cwd(void) {
    static char buf[MAX_PATH];
    return getcwd(buf, MAX_PATH);
}

int pl_set_cwd(const char *path) {
    return chdir(path);
}

char *pl_home_dir(void) {
    char *home = getenv("HOME");
    if (!home) home = getenv("USERPROFILE");
    return home ? xstrdup(home) : NULL;
}

char *pl_path_join(const char *a, const char *b) {
    static char buf[MAX_PATH];
    if (!a || !a[0]) return xstrdup(b);
    if (!b || !b[0]) return xstrdup(a);
    if (b[0] == '/') return xstrdup(b);
    snprintf(buf, MAX_PATH, "%s/%s", a, b);
    return xstrdup(buf);
}

int pl_list_dir(const char *path, PlatformDirEntry **entries, int *count) {
    DIR *dir = opendir(path);
    if (!dir) return -1;
    
    int cap = 128;
    *entries = xmalloc(cap * sizeof(PlatformDirEntry));
    *count = 0;
    
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (de->d_name[0] == '.' && de->d_name[1] == '\0') continue;
        
        if (*count >= cap) {
            cap *= 2;
            *entries = xrealloc(*entries, cap * sizeof(PlatformDirEntry));
        }
        
        PlatformDirEntry *e = &(*entries)[*count];
        strncpy(e->name, de->d_name, MAX_NAME - 1);
        e->name[MAX_NAME - 1] = '\0';
        e->type = (de->d_type == DT_DIR) ? ENTRY_DIR :
                  (de->d_type == DT_LNK) ? ENTRY_SYMLINK : ENTRY_FILE;
        
        char *fullpath = pl_path_join(path, de->d_name);
        struct stat st;
        if (stat(fullpath, &st) == 0) {
            e->size = st.st_size;
            e->mtime = st.st_mtime;
            e->type = S_ISDIR(st.st_mode) ? ENTRY_DIR : ENTRY_FILE;
        } else {
            e->size = 0;
            e->mtime = 0;
        }
        free(fullpath);
        (*count)++;
    }
    
    closedir(dir);
    qsort(*entries, *count, sizeof(PlatformDirEntry), entry_cmp);
    return 0;
}

int pl_is_dir(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int pl_file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

uint64_t pl_file_size(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) ? st.st_size : 0;
}

time_t pl_file_mtime(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) ? st.st_mtime : 0;
}

int pl_copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }
    
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { fclose(in); fclose(out); return -1; }
    }
    
    fclose(in);
    fclose(out);
    
    struct stat st;
    if (stat(src, &st) == 0) {
        chmod(dst, st.st_mode & 0777);
    }
    return 0;
}

int pl_copy_dir(const char *src, const char *dst) {
    if (mkdir(dst, 0755) != 0 && errno != EEXIST) return -1;
    
    PlatformDirEntry *entries;
    int count;
    if (pl_list_dir(src, &entries, &count) != 0) return -1;
    
    for (int i = 0; i < count; i++) {
        char *src_path = pl_path_join(src, entries[i].name);
        char *dst_path = pl_path_join(dst, entries[i].name);
        
        int ret = (entries[i].type == ENTRY_DIR) ?
                  pl_copy_dir(src_path, dst_path) :
                  pl_copy_file(src_path, dst_path);
        
        free(src_path);
        free(dst_path);
        if (ret != 0) { free(entries); return -1; }
    }
    free(entries);
    return 0;
}

int pl_move(const char *src, const char *dst) {
    return rename(src, dst);
}

int pl_delete(const char *path) {
    if (pl_is_dir(path)) {
        PlatformDirEntry *entries;
        int count;
        if (pl_list_dir(path, &entries, &count) != 0) return -1;
        
        for (int i = 0; i < count; i++) {
            if (strcmp(entries[i].name, ".") == 0 || strcmp(entries[i].name, "..") == 0) continue;
            char *sub = pl_path_join(path, entries[i].name);
            pl_delete(sub);
            free(sub);
        }
        free(entries);
        return rmdir(path);
    }
    return unlink(path);
}

int pl_mkdir(const char *path) {
    return (mkdir(path, 0755) == 0 || errno == EEXIST) ? 0 : -1;
}

int pl_is_text_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '\0') return 0;
        if ((unsigned char)buf[i] < 32 && buf[i] != '\n' && buf[i] != '\r' && buf[i] != '\t') {
            return 0;
        }
    }
    return 1;
}

char *pl_read_file(const char *path, size_t max_bytes, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    
    size_t len = pl_file_size(path);
    if (len > max_bytes) len = max_bytes;
    
    char *buf = xmalloc(len + 1);
    size_t n = fread(buf, 1, len, f);
    buf[n] = '\0';
    fclose(f);
    
    if (out_len) *out_len = n;
    return buf;
}

int pl_copy_file_progress(const char *src, const char *dst, ProgressCallback cb, void *userdata) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }
    
    uint64_t total = pl_file_size(src);
    uint64_t done = 0;
    
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return -1;
        }
        done += n;
        if (cb && !cb(done, total, dst, userdata)) {
            fclose(in);
            fclose(out);
            return -1;
        }
    }
    
    fclose(in);
    fclose(out);
    
    struct stat st;
    if (stat(src, &st) == 0) {
        chmod(dst, st.st_mode & 0777);
    }
    return 0;
}

int pl_copy_dir_progress(const char *src, const char *dst, ProgressCallback cb, void *userdata) {
    if (mkdir(dst, 0755) != 0 && errno != EEXIST) return -1;
    
    PlatformDirEntry *entries;
    int count;
    if (pl_list_dir(src, &entries, &count) != 0) return -1;
    
    for (int i = 0; i < count; i++) {
        char *src_path = pl_path_join(src, entries[i].name);
        char *dst_path = pl_path_join(dst, entries[i].name);
        
        int ret;
        if (entries[i].type == ENTRY_DIR) {
            ret = pl_copy_dir_progress(src_path, dst_path, cb, userdata);
        } else {
            ret = pl_copy_file_progress(src_path, dst_path, cb, userdata);
        }
        
        free(src_path);
        free(dst_path);
        if (ret != 0) { free(entries); return -1; }
    }
    free(entries);
    return 0;
}

#endif