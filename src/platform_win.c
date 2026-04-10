#ifdef _WIN32

#include "platform.h"
#include <windows.h>
#include <shlobj.h>
#include <direct.h>

static int entry_cmp(const void *a, const void *b) {
    const PlatformDirEntry *ea = a, *eb = b;
    if (ea->type != eb->type) {
        return (ea->type == ENTRY_DIR) ? -1 : 1;
    }
    return _stricmp(ea->name, eb->name);
}

int pl_init(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    return 0;
}

void pl_cleanup(void) {}

char *pl_get_cwd(void) {
    static char buf[MAX_PATH];
    return _getcwd(buf, MAX_PATH);
}

int pl_set_cwd(const char *path) {
    return _chdir(path);
}

char *pl_home_dir(void) {
    char buf[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, buf) == S_OK) {
        return xstrdup(buf);
    }
    char *home = getenv("USERPROFILE");
    return home ? xstrdup(home) : NULL;
}

char *pl_path_join(const char *a, const char *b) {
    static char buf[MAX_PATH];
    if (!a || !a[0]) return xstrdup(b);
    if (!b || !b[0]) return xstrdup(a);
    if (b[0] == '\\' || b[0] == '/' || (b[0] && b[1] == ':') || (b[0] && b[1] == '\\')) return xstrdup(b);
    snprintf(buf, MAX_PATH, "%s\\%s", a, b);
    return xstrdup(buf);
}

int pl_list_dir(const char *path, PlatformDirEntry **entries, int *count) {
    char pattern[MAX_PATH];
    snprintf(pattern, MAX_PATH, "%s\\*", path);
    
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return -1;
    
    int cap = 128;
    *entries = xmalloc(cap * sizeof(PlatformDirEntry));
    *count = 0;
    
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        
        if (*count >= cap) {
            cap *= 2;
            *entries = xrealloc(*entries, cap * sizeof(PlatformDirEntry));
        }
        
        PlatformDirEntry *e = &(*entries)[*count];
        strncpy(e->name, fd.cFileName, MAX_NAME - 1);
        e->name[MAX_NAME - 1] = '\0';
        e->type = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? ENTRY_DIR : ENTRY_FILE;
        e->size = ((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        
        ULARGE_INTEGER ull;
        ull.LowPart = fd.ftLastWriteTime.dwLowDateTime;
        ull.HighPart = fd.ftLastWriteTime.dwHighDateTime;
        e->mtime = (time_t)(ull.QuadPart / 10000000ULL - 11644473600ULL);
        
        (*count)++;
    } while (FindNextFileA(h, &fd));
    
    FindClose(h);
    qsort(*entries, *count, sizeof(PlatformDirEntry), entry_cmp);
    return 0;
}

int pl_is_dir(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

int pl_file_exists(const char *path) {
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

uint64_t pl_file_size(const char *path) {
    WIN32_FILE_ATTRIBUTE_DATA fd;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fd)) return 0;
    ULARGE_INTEGER size;
    size.LowPart = fd.nFileSizeLow;
    size.HighPart = fd.nFileSizeHigh;
    return size.QuadPart;
}

time_t pl_file_mtime(const char *path) {
    WIN32_FILE_ATTRIBUTE_DATA fd;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fd)) return 0;
    ULARGE_INTEGER ull;
    ull.LowPart = fd.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = fd.ftLastWriteTime.dwHighDateTime;
    return (time_t)(ull.QuadPart / 10000000ULL - 11644473600ULL);
}

int pl_copy_file(const char *src, const char *dst) {
    return CopyFileA(src, dst, FALSE) ? 0 : -1;
}

int pl_copy_dir(const char *src, const char *dst) {
    if (!CreateDirectoryA(dst, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return -1;
    
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
    return MoveFileExA(src, dst, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
}

int pl_delete(const char *path) {
    if (pl_is_dir(path)) {
        PlatformDirEntry *entries;
        int count;
        if (pl_list_dir(path, &entries, &count) != 0) return -1;
        for (int i = 0; i < count; i++) {
            char *sub = pl_path_join(path, entries[i].name);
            pl_delete(sub);
            free(sub);
        }
        free(entries);
        return RemoveDirectoryA(path) ? 0 : -1;
    }
    return DeleteFileA(path) ? 0 : -1;
}

int pl_mkdir(const char *path) {
    return (CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) ? 0 : -1;
}

int pl_is_text_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '\0') return 0;
        if ((unsigned char)buf[i] < 32 && buf[i] != '\n' && buf[i] != '\r' && buf[i] != '\t') return 0;
    }
    return 1;
}

char *pl_read_file(const char *path, size_t max_bytes, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    uint64_t size = pl_file_size(path);
    size_t len = (size > max_bytes) ? max_bytes : (size_t)size;
    char *buf = xmalloc(len + 1);
    size_t n = fread(buf, 1, len, f);
    buf[n] = '\0';
    fclose(f);
    if (out_len) *out_len = n;
    return buf;
}

int pl_copy_file_progress(const char *src, const char *dst, ProgressCallback cb, void *userdata) {
    return pl_copy_file(src, dst);
}

int pl_copy_dir_progress(const char *src, const char *dst, ProgressCallback cb, void *userdata) {
    return pl_copy_dir(src, dst);
}

#endif