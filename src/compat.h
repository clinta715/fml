#ifndef COMPAT_H
#define COMPAT_H

#ifdef _WIN32
  #define FML_STRCASECMP _stricmp
  #define FML_STRNCASECMP _strnicmp
  #define FML_PATHSEP '\\'
  #define FML_PATHSEP_STR "\\"
#else
  #define FML_STRCASECMP strcasecmp
  #define FML_STRNCASECMP strncasecmp
  #define FML_PATHSEP '/'
  #define FML_PATHSEP_STR "/"
#endif

#ifdef _WIN32
  #include <curses.h>
#else
  #include <ncurses.h>
#endif

#include <string.h>

static inline const char *fml_basename(const char *path) {
    const char *p = strrchr(path, '/');
    const char *q = strrchr(path, '\\');
    if (q && (!p || q > p)) p = q;
    return p ? p + 1 : path;
}

static inline int fml_fnmatch(const char *pattern, const char *string) {
    const char *p = pattern;
    const char *s = string;
    const char *star_p = NULL;
    const char *star_s = NULL;

    while (*s) {
        if (*p == '*') {
            star_p = p++;
            star_s = s;
            continue;
        }
        if (*p == '?') {
            p++;
            s++;
            continue;
        }
        if (*p == '[') {
            p++;
            int negate = 0;
            if (*p == '!') { negate = 1; p++; }
            int matched = 0;
            while (*p && *p != ']') {
                if (p[1] == '-' && p[2] && p[2] != ']') {
                    if (*s >= *p && *s <= p[2]) matched = 1;
                    p += 3;
                } else {
                    if (*p == *s) matched = 1;
                    p++;
                }
            }
            if (*p == ']') p++;
            if (negate) matched = !matched;
            if (!matched) goto backtrack;
            s++;
            continue;
        }
        if (*p == *s) {
            p++;
            s++;
            continue;
        }
backtrack:
        if (star_p) {
            p = star_p + 1;
            s = ++star_s;
            continue;
        }
        return 1;
    }
    while (*p == '*') p++;
    return (*p != '\0');
}

static inline int fml_has_parent_path(const char *path) {
#ifdef _WIN32
    if (!path[0]) return 0;
    if (path[1] == ':' && path[2] == '\\' && path[3] == '\0') return 0;
    if (path[1] == ':' && path[2] == '/' && path[3] == '\0') return 0;
    if ((path[0] == '\\' || path[0] == '/') && path[1] == '\0') return 0;
    if (path[0] == '\\' && path[1] == '\\' && strchr(path + 2, '\\') == NULL) return 0;
    return 1;
#else
    return strcmp(path, "/") != 0;
#endif
}

#ifdef _WIN32
  #define FML_STRTOK_R strtok_s
#else
  #define FML_STRTOK_R strtok_r
#endif

#ifdef _WIN32
  #define FML_ALLOCA _alloca
  #include <malloc.h>
#else
  #define FML_ALLOCA alloca
  #include <alloca.h>
#endif

#endif
