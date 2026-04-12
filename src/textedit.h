#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include "fml.h"
#include <ncurses.h>
#include <stdbool.h>

#define MAX_UNDO 4096
#define MAX_BOOKMARKS 256
#define TAB_SIZE 4
#define MAX_SEARCH_LEN 1024

typedef enum {
    UNDO_INSERT_CHAR,
    UNDO_DELETE_CHAR,
    UNDO_INSERT_BLOCK,
    UNDO_DELETE_BLOCK,
    UNDO_SPLIT_LINE,
    UNDO_JOIN_LINE,
    UNDO_REPLACE
} UndoType;

typedef struct {
    UndoType type;
    int line;
    int col;
    char *text;
    int text_len;
    int line2;
    int col2;
} UndoAction;

typedef struct {
    char *text;
    size_t len;
    size_t capacity;
} TextLine;

typedef enum {
    SYNTAX_NONE,
    SYNTAX_C,
    SYNTAX_PYTHON,
    SYNTAX_SHELL,
    SYNTAX_JAVASCRIPT,
    SYNTAX_JSON,
    SYNTAX_XML,
    SYNTAX_MARKDOWN,
    SYNTAX_RUST,
    SYNTAX_GO,
    SYNTAX_CSS,
    SYNTAX_COUNT
} SyntaxLang;

typedef struct {
    bool active;
    int start_line;
    int start_col;
    int end_line;
    int end_col;
} TextSelection;

typedef struct {
    char path[MAX_PATH];
    TextLine *lines;
    int line_count;
    int line_capacity;
    int cursor_line;
    int cursor_col;
    int scroll_line;
    int scroll_col;
    TextSelection sel;
    UndoAction undo_stack[MAX_UNDO];
    int undo_count;
    UndoAction redo_stack[MAX_UNDO];
    int redo_count;
    int bookmarks[MAX_BOOKMARKS];
    int bookmark_count;
    bool modified;
    bool insert_mode;
    bool word_wrap;
    SyntaxLang syntax;
    int width;
    int height;
    char status_msg[256];
    char search_str[MAX_SEARCH_LEN];
    bool search_active;
    int *search_matches;
    int search_match_count;
    int search_match_capacity;
    int search_current;
    int undo_group_start;
} TextEditor;

int textedit_open(TextEditor *ed, const char *path);
void textedit_close(TextEditor *ed);
void textedit_draw(TextEditor *ed, WINDOW *win, int w, int h);
void textedit_handle_key(TextEditor *ed, int key);
int textedit_save(TextEditor *ed);
int textedit_save_as(TextEditor *ed, const char *path);
void textedit_goto_line(TextEditor *ed, int line);
void textedit_find(TextEditor *ed, const char *str);
void textedit_find_next(TextEditor *ed);
void textedit_find_prev(TextEditor *ed);
void textedit_replace(TextEditor *ed, const char *find_str, const char *replace_str, bool all);

#endif
