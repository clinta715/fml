#include "archive.h"
#include "platform.h"
#include "compat.h"
#include <stdlib.h>

static const char *supported_exts[] = {
    ".zip", ".tar", ".tar.gz", ".tgz", ".tar.bz2", ".tbz2",
    ".tar.xz", ".txz", ".7z", ".rar", NULL
};

bool archive_is_supported(const char *filename) {
    if (!filename) return false;
    const char *ext = strrchr(filename, '.');
    if (!ext) return false;
    
    size_t flen = strlen(filename);
    
    for (int i = 0; supported_exts[i]; i++) {
        size_t elen = strlen(supported_exts[i]);
        
        if (elen == 4 && FML_STRCASECMP(ext, supported_exts[i]) == 0) {
            return true;
        }
        
        if (flen > elen) {
            if (FML_STRCASECMP(filename + flen - elen, supported_exts[i]) == 0) {
                return true;
            }
        }
    }
    return false;
}

int archive_extract(const char *archive_path, const char *dest_dir) {
    char cmd[MAX_PATH * 3];
    const char *ext = strrchr(archive_path, '.');
    size_t plen = strlen(archive_path);
    
    if (plen > 7 && FML_STRCASECMP(archive_path + plen - 7, ".tar.gz") == 0) {
        ext = ".tar.gz";
    } else if (plen > 8 && FML_STRCASECMP(archive_path + plen - 8, ".tar.bz2") == 0) {
        ext = ".tar.bz2";
    } else if (plen > 7 && FML_STRCASECMP(archive_path + plen - 7, ".tar.xz") == 0) {
        ext = ".tar.xz";
    }
    
#ifdef _WIN32
    if (FML_STRCASECMP(ext, ".zip") == 0) {
        snprintf(cmd, sizeof(cmd), "powershell -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".tar.gz") == 0 || FML_STRCASECMP(ext, ".tgz") == 0) {
        snprintf(cmd, sizeof(cmd), "tar -xzf \"%s\" -C \"%s\"", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".tar.bz2") == 0 || FML_STRCASECMP(ext, ".tbz2") == 0) {
        snprintf(cmd, sizeof(cmd), "tar -xjf \"%s\" -C \"%s\"", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".tar.xz") == 0 || FML_STRCASECMP(ext, ".txz") == 0) {
        snprintf(cmd, sizeof(cmd), "tar -xJf \"%s\" -C \"%s\"", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".tar") == 0) {
        snprintf(cmd, sizeof(cmd), "tar -xf \"%s\" -C \"%s\"", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".7z") == 0) {
        snprintf(cmd, sizeof(cmd), "7z x \"%s\" -o\"%s\" -y 2>NUL", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".rar") == 0) {
        snprintf(cmd, sizeof(cmd), "unrar x \"%s\" \"%s\\\\\" 2>NUL", archive_path, dest_dir);
    } else {
        return -1;
    }
#else
    if (FML_STRCASECMP(ext, ".zip") == 0) {
        snprintf(cmd, sizeof(cmd), "unzip -o '%s' -d '%s'", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".tar.gz") == 0 || FML_STRCASECMP(ext, ".tgz") == 0) {
        snprintf(cmd, sizeof(cmd), "tar -xzf '%s' -C '%s'", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".tar.bz2") == 0 || FML_STRCASECMP(ext, ".tbz2") == 0) {
        snprintf(cmd, sizeof(cmd), "tar -xjf '%s' -C '%s'", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".tar.xz") == 0 || FML_STRCASECMP(ext, ".txz") == 0) {
        snprintf(cmd, sizeof(cmd), "tar -xJf '%s' -C '%s'", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".tar") == 0) {
        snprintf(cmd, sizeof(cmd), "tar -xf '%s' -C '%s'", archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".7z") == 0) {
        snprintf(cmd, sizeof(cmd), "7z x '%s' -o'%s' -y 2>/dev/null || unzip -o '%s' -d '%s'", 
                 archive_path, dest_dir, archive_path, dest_dir);
    } else if (FML_STRCASECMP(ext, ".rar") == 0) {
        snprintf(cmd, sizeof(cmd), "unrar x '%s' '%s/' 2>/dev/null || unzip -o '%s' -d '%s'", 
                 archive_path, dest_dir, archive_path, dest_dir);
    } else {
        return -1;
    }
#endif
    
    return system(cmd);
}
