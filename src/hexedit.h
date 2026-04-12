#ifndef HEXEDIT_H
#define HEXEDIT_H

#include "fml.h"
#include <ncurses.h>

typedef struct {
    char path[MAX_PATH];
    unsigned char *data;
    size_t file_size;
    size_t capacity;
    size_t cursor;
    int scroll_offset;
    bool modified;
    int hex_nibble;
    bool editing_ascii;
    bool insert_mode;
    int width;
    int height;
    char status_msg[256];
} HexEditor;

int hexedit_open(HexEditor *ed, const char *path);
void hexedit_close(HexEditor *ed);
void hexedit_draw(HexEditor *ed, WINDOW *win, int w, int h);
void hexedit_handle_key(HexEditor *ed, int key);
int hexedit_save(HexEditor *ed);
int hexedit_save_as(HexEditor *ed, const char *path);
void hexedit_goto(HexEditor *ed, size_t offset);
void hexedit_find(HexEditor *ed, const unsigned char *pattern, size_t pat_len);

#endif
