#ifndef FML_H
#define FML_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define VERSION "0.3.0"
#ifndef FML_MAX_PATH
#define FML_MAX_PATH 4096
#endif
#ifdef MAX_PATH
#undef MAX_PATH
#endif
#define MAX_PATH FML_MAX_PATH
#define MAX_NAME 256
#define MAX_ENTRIES 8192
#define MAX_SEARCH 256
#define MAX_LINE 1024

#define COLOR_BORDER 1
#define COLOR_SELECTED 2
#define COLOR_STATUS 3
#define COLOR_PREVIEW 4
#define COLOR_DIR 5
#define COLOR_PARENT 6
#define COLOR_SYMLINK 7
#define COLOR_EXEC 8
#define COLOR_FILE 9
#define COLOR_HIDDEN 10
#define COLOR_ARCHIVE 11
#define COLOR_MEDIA 12
#define COLOR_CODE 13

typedef enum {
    PANEL_LEFT = 0,
    PANEL_RIGHT = 1,
    PANEL_COUNT = 2
} PanelId;

typedef enum {
    MODE_NORMAL,
    MODE_SEARCH,
    MODE_INPUT,
    MODE_PREVIEW_FULLSCREEN,
    MODE_HEXEDIT,
    MODE_TEXTEDIT
} UIMode;

typedef enum {
    ENTRY_FILE,
    ENTRY_DIR,
    ENTRY_SYMLINK,
    ENTRY_PARENT
} EntryType;

typedef enum {
    SORT_NAME,
    SORT_SIZE,
    SORT_DATE,
    SORT_EXT
} SortMode;

typedef struct {
    char name[MAX_NAME];
    char path[MAX_PATH];
    EntryType type;
    uint64_t size;
    time_t mtime;
    bool selected;
} DirEntry;

typedef struct {
    char path[MAX_PATH];
    DirEntry *entries;
    int count;
    int cursor;
    int scroll_offset;
    char search_filter[MAX_SEARCH];
    bool show_hidden;
    SortMode sort_mode;
    bool sort_reverse;
} Panel;

typedef struct {
    Panel panels[PANEL_COUNT];
    PanelId active;
    UIMode mode;
    char input_buffer[MAX_PATH];
    int input_len;
    char status_msg[MAX_LINE];
    bool running;
} AppState;

extern AppState g_state;

void die(const char *msg);
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);

#endif