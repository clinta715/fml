#include "textedit.h"
#include "platform.h"
#include "ui.h"
#include "clipboard.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define COLOR_SYN_KEYWORD 30
#define COLOR_SYN_STRING  31
#define COLOR_SYN_COMMENT 32
#define COLOR_SYN_NUMBER  33
#define COLOR_SYN_TYPE    34
#define COLOR_SYN_MATCH   35
#define COLOR_SYN_SPECIAL 36

#define LINE_INIT_CAP 256
#define GUTTER_MIN 5

typedef struct {
    const char **keywords;
    int keyword_count;
    const char **types;
    int type_count;
    const char **specials;
    int special_count;
    char single_line_comment[4];
    char multi_line_start[4];
    char multi_line_end[4];
} SyntaxDef;

static const char *c_keywords[] = {
    "auto","break","case","const","continue","default","do","else","enum",
    "extern","for","goto","if","inline","register","restrict","return",
    "sizeof","static","struct","switch","typedef","union","volatile","while",
    "_Bool","_Complex","_Imaginary","NULL","true","false","NULL",
    NULL
};
static const char *c_types[] = {
    "void","char","short","int","long","float","double","unsigned","signed",
    "size_t","ssize_t","uint8_t","uint16_t","uint32_t","uint64_t",
    "int8_t","int16_t","int32_t","int64_t","bool","FILE",
    NULL
};

static const char *py_keywords[] = {
    "and","as","assert","async","await","break","class","continue",
    "def","del","elif","else","except","finally","for","from",
    "global","if","import","in","is","lambda","nonlocal","not",
    "or","pass","raise","return","try","while","with","yield",
    "True","False","None","print","self",
    NULL
};
static const char *py_types[] = {
    "int","float","str","list","dict","tuple","set","bool","bytes",
    "object","type","range","enumerate","zip","map","filter",
    NULL
};

static const char *js_keywords[] = {
    "async","await","break","case","catch","class","const","continue",
    "debugger","default","delete","do","else","export","extends",
    "finally","for","from","function","if","import","in","instanceof",
    "let","new","of","return","static","super","switch","this",
    "throw","try","typeof","var","void","while","with","yield",
    "true","false","null","undefined",
    NULL
};
static const char *js_types[] = {
    "Array","Boolean","Date","Error","Function","Map","Number",
    "Object","Promise","RegExp","Set","String","Symbol","WeakMap",
    NULL
};

static const char *rs_keywords[] = {
    "as","async","await","break","const","continue","crate","dyn",
    "else","enum","extern","fn","for","if","impl","in","let",
    "loop","match","mod","move","mut","pub","ref","return",
    "self","Self","static","struct","super","trait","type","unsafe",
    "use","where","while","true","false",
    NULL
};
static const char *rs_types[] = {
    "i8","i16","i32","i64","i128","u8","u16","u32","u64","u128",
    "f32","f64","bool","char","str","String","Vec","Box","Option",
    "Result","Ok","Err","Some","None","println",
    NULL
};

static const char *go_keywords[] = {
    "break","case","chan","const","continue","default","defer","else",
    "fallthrough","for","func","go","goto","if","import","interface",
    "map","package","range","return","select","struct","switch","type",
    "var","true","false","nil","append","cap","close","copy","delete",
    "len","make","new","panic","println","print","recover",
    NULL
};
static const char *go_types[] = {
    "bool","byte","complex64","complex128","error","float32","float64",
    "int","int8","int16","int32","int64","rune","string",
    "uint","uint8","uint16","uint32","uint64","uintptr",
    NULL
};

static const char *sh_keywords[] = {
    "if","then","else","elif","fi","case","esac","for","while","until",
    "do","done","in","function","select","time","coproc","return",
    "exit","break","continue","declare","export","local","readonly",
    "typeset","unset","source","alias","echo","cd","pwd","ls","mkdir",
    "rm","cp","mv","cat","grep","sed","awk","find","chmod","chown",
    "true","false","test","null",
    NULL
};

static const char *css_keywords[] = {
    "inherit","initial","unset","none","auto","block","inline","flex",
    "grid","absolute","relative","fixed","sticky","hidden","visible",
    "solid","dashed","dotted","center","left","right","top","bottom",
    "bold","normal","italic","underline","pointer","important",
    NULL
};

static SyntaxDef syntax_defs[SYNTAX_COUNT] = {0};

static void init_syntax_defs(void) {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    syntax_defs[SYNTAX_C].keywords = c_keywords;
    syntax_defs[SYNTAX_C].keyword_count = 0;
    for (int i = 0; c_keywords[i]; i++) syntax_defs[SYNTAX_C].keyword_count++;
    syntax_defs[SYNTAX_C].types = c_types;
    syntax_defs[SYNTAX_C].type_count = 0;
    for (int i = 0; c_types[i]; i++) syntax_defs[SYNTAX_C].type_count++;
    strncpy(syntax_defs[SYNTAX_C].single_line_comment, "//", 4);
    strncpy(syntax_defs[SYNTAX_C].multi_line_start, "/*", 4);
    strncpy(syntax_defs[SYNTAX_C].multi_line_end, "*/", 4);

    syntax_defs[SYNTAX_PYTHON].keywords = py_keywords;
    syntax_defs[SYNTAX_PYTHON].keyword_count = 0;
    for (int i = 0; py_keywords[i]; i++) syntax_defs[SYNTAX_PYTHON].keyword_count++;
    syntax_defs[SYNTAX_PYTHON].types = py_types;
    syntax_defs[SYNTAX_PYTHON].type_count = 0;
    for (int i = 0; py_types[i]; i++) syntax_defs[SYNTAX_PYTHON].type_count++;
    strncpy(syntax_defs[SYNTAX_PYTHON].single_line_comment, "#", 4);

    syntax_defs[SYNTAX_SHELL].keywords = sh_keywords;
    syntax_defs[SYNTAX_SHELL].keyword_count = 0;
    for (int i = 0; sh_keywords[i]; i++) syntax_defs[SYNTAX_SHELL].keyword_count++;
    strncpy(syntax_defs[SYNTAX_SHELL].single_line_comment, "#", 4);

    syntax_defs[SYNTAX_JAVASCRIPT].keywords = js_keywords;
    syntax_defs[SYNTAX_JAVASCRIPT].keyword_count = 0;
    for (int i = 0; js_keywords[i]; i++) syntax_defs[SYNTAX_JAVASCRIPT].keyword_count++;
    syntax_defs[SYNTAX_JAVASCRIPT].types = js_types;
    syntax_defs[SYNTAX_JAVASCRIPT].type_count = 0;
    for (int i = 0; js_types[i]; i++) syntax_defs[SYNTAX_JAVASCRIPT].type_count++;
    strncpy(syntax_defs[SYNTAX_JAVASCRIPT].single_line_comment, "//", 4);
    strncpy(syntax_defs[SYNTAX_JAVASCRIPT].multi_line_start, "/*", 4);
    strncpy(syntax_defs[SYNTAX_JAVASCRIPT].multi_line_end, "*/", 4);

    syntax_defs[SYNTAX_JSON].keywords = NULL;
    syntax_defs[SYNTAX_JSON].keyword_count = 0;
    syntax_defs[SYNTAX_JSON].types = NULL;
    syntax_defs[SYNTAX_JSON].type_count = 0;

    syntax_defs[SYNTAX_XML].keywords = NULL;
    syntax_defs[SYNTAX_XML].keyword_count = 0;
    syntax_defs[SYNTAX_XML].types = NULL;
    syntax_defs[SYNTAX_XML].type_count = 0;
    strncpy(syntax_defs[SYNTAX_XML].multi_line_start, "<!--", 4);
    strncpy(syntax_defs[SYNTAX_XML].multi_line_end, "-->", 4);

    syntax_defs[SYNTAX_MARKDOWN].keywords = NULL;
    syntax_defs[SYNTAX_MARKDOWN].keyword_count = 0;
    syntax_defs[SYNTAX_MARKDOWN].types = NULL;
    syntax_defs[SYNTAX_MARKDOWN].type_count = 0;

    syntax_defs[SYNTAX_RUST].keywords = rs_keywords;
    syntax_defs[SYNTAX_RUST].keyword_count = 0;
    for (int i = 0; rs_keywords[i]; i++) syntax_defs[SYNTAX_RUST].keyword_count++;
    syntax_defs[SYNTAX_RUST].types = rs_types;
    syntax_defs[SYNTAX_RUST].type_count = 0;
    for (int i = 0; rs_types[i]; i++) syntax_defs[SYNTAX_RUST].type_count++;
    strncpy(syntax_defs[SYNTAX_RUST].single_line_comment, "//", 4);
    strncpy(syntax_defs[SYNTAX_RUST].multi_line_start, "/*", 4);
    strncpy(syntax_defs[SYNTAX_RUST].multi_line_end, "*/", 4);

    syntax_defs[SYNTAX_GO].keywords = go_keywords;
    syntax_defs[SYNTAX_GO].keyword_count = 0;
    for (int i = 0; go_keywords[i]; i++) syntax_defs[SYNTAX_GO].keyword_count++;
    syntax_defs[SYNTAX_GO].types = go_types;
    syntax_defs[SYNTAX_GO].type_count = 0;
    for (int i = 0; go_types[i]; i++) syntax_defs[SYNTAX_GO].type_count++;
    strncpy(syntax_defs[SYNTAX_GO].single_line_comment, "//", 4);

    syntax_defs[SYNTAX_CSS].keywords = css_keywords;
    syntax_defs[SYNTAX_CSS].keyword_count = 0;
    for (int i = 0; css_keywords[i]; i++) syntax_defs[SYNTAX_CSS].keyword_count++;
    strncpy(syntax_defs[SYNTAX_CSS].multi_line_start, "/*", 4);
    strncpy(syntax_defs[SYNTAX_CSS].multi_line_end, "*/", 4);
}

static SyntaxLang detect_syntax(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return SYNTAX_NONE;
    ext++;

    if (strcasecmp(ext, "c") == 0 || strcasecmp(ext, "h") == 0) return SYNTAX_C;
    if (strcasecmp(ext, "py") == 0 || strcasecmp(ext, "pyw") == 0) return SYNTAX_PYTHON;
    if (strcasecmp(ext, "sh") == 0 || strcasecmp(ext, "bash") == 0 ||
        strcasecmp(ext, "zsh") == 0) return SYNTAX_SHELL;
    if (strcasecmp(ext, "js") == 0 || strcasecmp(ext, "jsx") == 0 ||
        strcasecmp(ext, "ts") == 0 || strcasecmp(ext, "tsx") == 0 ||
        strcasecmp(ext, "mjs") == 0) return SYNTAX_JAVASCRIPT;
    if (strcasecmp(ext, "json") == 0) return SYNTAX_JSON;
    if (strcasecmp(ext, "xml") == 0 || strcasecmp(ext, "html") == 0 ||
        strcasecmp(ext, "htm") == 0 || strcasecmp(ext, "svg") == 0) return SYNTAX_XML;
    if (strcasecmp(ext, "md") == 0 || strcasecmp(ext, "markdown") == 0) return SYNTAX_MARKDOWN;
    if (strcasecmp(ext, "rs") == 0) return SYNTAX_RUST;
    if (strcasecmp(ext, "go") == 0) return SYNTAX_GO;
    if (strcasecmp(ext, "css") == 0 || strcasecmp(ext, "scss") == 0 ||
        strcasecmp(ext, "less") == 0) return SYNTAX_CSS;
    return SYNTAX_NONE;
}

static void line_ensure_cap(TextLine *l, size_t needed) {
    if (needed <= l->capacity) return;
    size_t new_cap = l->capacity;
    while (new_cap < needed) new_cap *= 2;
    l->text = xrealloc(l->text, new_cap);
    l->capacity = new_cap;
}

static void line_insert_char(TextLine *l, int pos, char ch) {
    if (pos < 0) pos = 0;
    if ((size_t)pos > l->len) pos = (int)l->len;
    line_ensure_cap(l, l->len + 2);
    memmove(l->text + pos + 1, l->text + pos, l->len - pos + 1);
    l->text[pos] = ch;
    l->len++;
}

static void line_delete_char(TextLine *l, int pos) {
    if (pos < 0 || (size_t)pos >= l->len) return;
    memmove(l->text + pos, l->text + pos + 1, l->len - pos);
    l->len--;
}

static void line_insert_str(TextLine *l, int pos, const char *str, int slen) {
    if (slen <= 0) return;
    if (pos < 0) pos = 0;
    if ((size_t)pos > l->len) pos = (int)l->len;
    line_ensure_cap(l, l->len + slen + 1);
    memmove(l->text + pos + slen, l->text + pos, l->len - pos + 1);
    memcpy(l->text + pos, str, slen);
    l->len += slen;
}

static void line_delete_range(TextLine *l, int start, int end) {
    if (start < 0) start = 0;
    if (end > (int)l->len) end = (int)l->len;
    if (start >= end) return;
    int del_len = end - start;
    memmove(l->text + start, l->text + end, l->len - end + 1);
    l->len -= del_len;
}

static void ensure_lines(TextEditor *ed, int needed) {
    if (needed <= ed->line_capacity) return;
    int new_cap = ed->line_capacity ? ed->line_capacity : 128;
    while (new_cap < needed) new_cap *= 2;
    ed->lines = xrealloc(ed->lines, sizeof(TextLine) * new_cap);
    ed->line_capacity = new_cap;
}

static void insert_line(TextEditor *ed, int at, const char *text, size_t len) {
    ensure_lines(ed, ed->line_count + 1);
    if (at < ed->line_count) {
        memmove(&ed->lines[at + 1], &ed->lines[at], sizeof(TextLine) * (ed->line_count - at));
    }
    ed->lines[at].text = NULL;
    ed->lines[at].len = 0;
    ed->lines[at].capacity = 0;
    line_ensure_cap(&ed->lines[at], len < LINE_INIT_CAP ? LINE_INIT_CAP : len + 1);
    if (len > 0) memcpy(ed->lines[at].text, text, len);
    ed->lines[at].text[len] = '\0';
    ed->lines[at].len = len;
    ed->line_count++;
}

static void delete_line(TextEditor *ed, int at) {
    if (at < 0 || at >= ed->line_count) return;
    free(ed->lines[at].text);
    if (at < ed->line_count - 1) {
        memmove(&ed->lines[at], &ed->lines[at + 1], sizeof(TextLine) * (ed->line_count - at - 1));
    }
    ed->line_count--;
}

static void push_undo(TextEditor *ed, UndoAction *a) {
    if (ed->undo_count >= MAX_UNDO) {
        free(ed->undo_stack[0].text);
        memmove(&ed->undo_stack[0], &ed->undo_stack[1], sizeof(UndoAction) * (MAX_UNDO - 1));
        ed->undo_count--;
    }
    ed->undo_stack[ed->undo_count++] = *a;
    ed->redo_count = 0;
}

static void push_redo(TextEditor *ed, UndoAction *a) {
    if (ed->redo_count >= MAX_UNDO) {
        free(ed->redo_stack[0].text);
        memmove(&ed->redo_stack[0], &ed->redo_stack[1], sizeof(UndoAction) * (MAX_UNDO - 1));
        ed->redo_count--;
    }
    ed->redo_stack[ed->redo_count++] = *a;
}

static int get_indent(const char *text, size_t len) {
    int indent = 0;
    for (size_t i = 0; i < len; i++) {
        if (text[i] == ' ') indent++;
        else if (text[i] == '\t') indent += TAB_SIZE;
        else break;
    }
    return indent;
}

static void clamp_cursor(TextEditor *ed) {
    if (ed->cursor_line < 0) ed->cursor_line = 0;
    if (ed->cursor_line >= ed->line_count) ed->cursor_line = ed->line_count - 1;
    if (ed->cursor_line < 0) ed->cursor_line = 0;
    int max_col = (int)ed->lines[ed->cursor_line].len;
    if (!ed->insert_mode && max_col > 0) max_col--;
    if (ed->cursor_col > max_col) ed->cursor_col = max_col;
    if (ed->cursor_col < 0) ed->cursor_col = 0;
}

static void ensure_visible(TextEditor *ed, int visible_rows) {
    if (ed->cursor_line < ed->scroll_line) {
        ed->scroll_line = ed->cursor_line;
    }
    if (ed->cursor_line >= ed->scroll_line + visible_rows) {
        ed->scroll_line = ed->cursor_line - visible_rows + 1;
    }
    if (ed->scroll_line < 0) ed->scroll_line = 0;
}

int textedit_open(TextEditor *ed, const char *path) {
    memset(ed, 0, sizeof(TextEditor));
    strncpy(ed->path, path, MAX_PATH - 1);
    init_syntax_defs();
    ed->syntax = detect_syntax(path);

    size_t len;
    char *content = pl_read_file(path, SIZE_MAX, &len);
    if (!content) {
        ed->lines = xmalloc(sizeof(TextLine) * 128);
        ed->line_capacity = 128;
        ed->lines[0].text = xmalloc(LINE_INIT_CAP);
        ed->lines[0].text[0] = '\0';
        ed->lines[0].len = 0;
        ed->lines[0].capacity = LINE_INIT_CAP;
        ed->line_count = 1;
    } else {
        int line_cap = 128;
        int line_count = 0;
        ed->lines = xmalloc(sizeof(TextLine) * line_cap);

        char *p = content;
        char *line_start = content;
        while (*p) {
            if (*p == '\n') {
                size_t line_len = p - line_start;
                if (line_len > 0 && *(p - 1) == '\r') line_len--;
                if (line_count >= line_cap) {
                    line_cap *= 2;
                    ed->lines = xrealloc(ed->lines, sizeof(TextLine) * line_cap);
                }
                ed->lines[line_count].capacity = line_len < LINE_INIT_CAP ? LINE_INIT_CAP : line_len + 1;
                ed->lines[line_count].text = xmalloc(ed->lines[line_count].capacity);
                memcpy(ed->lines[line_count].text, line_start, line_len);
                ed->lines[line_count].text[line_len] = '\0';
                ed->lines[line_count].len = line_len;
                line_count++;
                line_start = p + 1;
            }
            p++;
        }
        if (line_start < p || line_count == 0) {
            size_t line_len = p - line_start;
            if (line_len > 0 && *(p - 1) == '\r') line_len--;
            if (line_count >= line_cap) {
                line_cap *= 2;
                ed->lines = xrealloc(ed->lines, sizeof(TextLine) * line_cap);
            }
            ed->lines[line_count].capacity = line_len < LINE_INIT_CAP ? LINE_INIT_CAP : line_len + 1;
            ed->lines[line_count].text = xmalloc(ed->lines[line_count].capacity);
            memcpy(ed->lines[line_count].text, line_start, line_len);
            ed->lines[line_count].text[line_len] = '\0';
            ed->lines[line_count].len = line_len;
            line_count++;
        }
        ed->line_count = line_count;
        ed->line_capacity = line_cap;
        free(content);
    }

    ed->cursor_line = 0;
    ed->cursor_col = 0;
    ed->scroll_line = 0;
    ed->scroll_col = 0;
    ed->modified = false;
    ed->insert_mode = false;
    ed->word_wrap = false;
    ed->status_msg[0] = '\0';
    ed->search_str[0] = '\0';
    ed->search_active = false;
    ed->search_matches = NULL;
    ed->search_match_count = 0;
    ed->search_match_capacity = 0;
    ed->search_current = -1;
    ed->bookmark_count = 0;
    ed->undo_count = 0;
    ed->redo_count = 0;
    ed->undo_group_start = 0;
    return 0;
}

void textedit_close(TextEditor *ed) {
    for (int i = 0; i < ed->line_count; i++) {
        free(ed->lines[i].text);
    }
    free(ed->lines);
    ed->lines = NULL;
    ed->line_count = 0;
    ed->line_capacity = 0;
    for (int i = 0; i < ed->undo_count; i++) free(ed->undo_stack[i].text);
    for (int i = 0; i < ed->redo_count; i++) free(ed->redo_stack[i].text);
    free(ed->search_matches);
    ed->search_matches = NULL;
    ed->search_match_count = 0;
    ed->search_match_capacity = 0;
}

int textedit_save(TextEditor *ed) {
    FILE *f = fopen(ed->path, "wb");
    if (!f) {
        snprintf(ed->status_msg, sizeof(ed->status_msg), "Error: cannot open file for writing");
        return -1;
    }
    for (int i = 0; i < ed->line_count; i++) {
        TextLine *l = &ed->lines[i];
        if (l->len > 0 && fwrite(l->text, 1, l->len, f) != l->len) {
            fclose(f);
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Error: write failed");
            return -1;
        }
        if (i < ed->line_count - 1) {
            if (fwrite("\n", 1, 1, f) != 1) {
                fclose(f);
                snprintf(ed->status_msg, sizeof(ed->status_msg), "Error: write failed");
                return -1;
            }
        }
    }
    fclose(f);
    ed->modified = false;
    snprintf(ed->status_msg, sizeof(ed->status_msg), "Saved %d lines", ed->line_count);
    return 0;
}

int textedit_save_as(TextEditor *ed, const char *path) {
    char old[MAX_PATH];
    strncpy(old, ed->path, MAX_PATH - 1);
    strncpy(ed->path, path, MAX_PATH - 1);
    ed->syntax = detect_syntax(path);
    int ret = textedit_save(ed);
    if (ret != 0) {
        strncpy(ed->path, old, MAX_PATH - 1);
    }
    return ret;
}

void textedit_goto_line(TextEditor *ed, int line) {
    if (line < 1) line = 1;
    if (line > ed->line_count) line = ed->line_count;
    ed->cursor_line = line - 1;
    ed->cursor_col = 0;
    ed->status_msg[0] = '\0';
}

static bool str_match_at(const char *haystack, size_t hlen, int pos, const char *needle, int nlen) {
    if (pos < 0 || pos + nlen > (int)hlen) return false;
    return memcmp(haystack + pos, needle, nlen) == 0;
}

static void update_search_matches(TextEditor *ed) {
    ed->search_match_count = 0;
    if (!ed->search_active || ed->search_str[0] == '\0') return;

    int slen = (int)strlen(ed->search_str);
    for (int i = 0; i < ed->line_count; i++) {
        TextLine *l = &ed->lines[i];
        for (int j = 0; j <= (int)l->len - slen; j++) {
            if (str_match_at(l->text, l->len, j, ed->search_str, slen)) {
                if (ed->search_match_count >= ed->search_match_capacity) {
                    ed->search_match_capacity = ed->search_match_capacity ? ed->search_match_capacity * 2 : 256;
                    ed->search_matches = xrealloc(ed->search_matches, sizeof(int) * ed->search_match_capacity * 2);
                }
                ed->search_matches[ed->search_match_count * 2] = i;
                ed->search_matches[ed->search_match_count * 2 + 1] = j;
                ed->search_match_count++;
                j += slen - 1;
            }
        }
    }
}

void textedit_find(TextEditor *ed, const char *str) {
    strncpy(ed->search_str, str, MAX_SEARCH_LEN - 1);
    ed->search_str[MAX_SEARCH_LEN - 1] = '\0';
    ed->search_active = true;
    ed->search_current = -1;
    update_search_matches(ed);
    textedit_find_next(ed);
}

void textedit_find_next(TextEditor *ed) {
    if (!ed->search_active || ed->search_match_count == 0) {
        snprintf(ed->status_msg, sizeof(ed->status_msg), "No matches found");
        return;
    }
    int slen = (int)strlen(ed->search_str);
    for (int i = ed->search_current + 1; i < ed->search_match_count; i++) {
        int ml = ed->search_matches[i * 2];
        int mc = ed->search_matches[i * 2 + 1];
        if (ml > ed->cursor_line || (ml == ed->cursor_line && mc >= ed->cursor_col)) {
            ed->cursor_line = ml;
            ed->cursor_col = mc + slen;
            ed->search_current = i;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Match %d/%d", i + 1, ed->search_match_count);
            return;
        }
    }
    ed->cursor_line = ed->search_matches[0];
    ed->cursor_col = ed->search_matches[1] + slen;
    ed->search_current = 0;
    snprintf(ed->status_msg, sizeof(ed->status_msg), "Match 1/%d (wrapped)", ed->search_match_count);
}

void textedit_find_prev(TextEditor *ed) {
    if (!ed->search_active || ed->search_match_count == 0) {
        snprintf(ed->status_msg, sizeof(ed->status_msg), "No matches found");
        return;
    }
    int slen = (int)strlen(ed->search_str);
    for (int i = ed->search_current - 1; i >= 0; i--) {
        int ml = ed->search_matches[i * 2];
        int mc = ed->search_matches[i * 2 + 1];
        if (ml < ed->cursor_line || (ml == ed->cursor_line && mc + slen <= ed->cursor_col)) {
            ed->cursor_line = ml;
            ed->cursor_col = mc + slen;
            ed->search_current = i;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Match %d/%d", i + 1, ed->search_match_count);
            return;
        }
    }
    int last = ed->search_match_count - 1;
    ed->cursor_line = ed->search_matches[last * 2];
    ed->cursor_col = ed->search_matches[last * 2 + 1] + slen;
    ed->search_current = last;
    snprintf(ed->status_msg, sizeof(ed->status_msg), "Match %d/%d (wrapped)", last + 1, ed->search_match_count);
}

void textedit_replace(TextEditor *ed, const char *find_str, const char *replace_str, bool all) {
    int flen = (int)strlen(find_str);
    int rlen = (int)strlen(replace_str);
    int count = 0;

    strncpy(ed->search_str, find_str, MAX_SEARCH_LEN - 1);
    ed->search_active = true;
    update_search_matches(ed);

    if (all) {
        for (int i = ed->line_count - 1; i >= 0; i--) {
            TextLine *l = &ed->lines[i];
            int j = (int)l->len - flen;
            while (j >= 0) {
                if (str_match_at(l->text, l->len, j, find_str, flen)) {
                    line_delete_range(l, j, j + flen);
                    line_insert_str(l, j, replace_str, rlen);
                    count++;
                    j -= flen;
                } else {
                    j--;
                }
            }
        }
        if (count > 0) {
            ed->modified = true;
            UndoAction a = {0};
            a.type = UNDO_REPLACE;
            a.text = xstrdup(find_str);
            a.text_len = flen;
            a.line = -1;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Replaced %d occurrences", count);
            push_undo(ed, &a);
        }
    } else {
        if (ed->search_match_count > 0 && ed->search_current >= 0) {
            int idx = ed->search_current;
            int ml = ed->search_matches[idx * 2];
            int mc = ed->search_matches[idx * 2 + 1];
            TextLine *l = &ed->lines[ml];
            line_delete_range(l, mc, mc + flen);
            line_insert_str(l, mc, replace_str, rlen);
            ed->cursor_line = ml;
            ed->cursor_col = mc + rlen;
            ed->modified = true;
            count = 1;
            UndoAction a = {0};
            a.type = UNDO_REPLACE;
            a.line = ml;
            a.col = mc;
            a.text = xstrdup(find_str);
            a.text_len = flen;
            a.line2 = rlen;
            push_undo(ed, &a);
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Replaced 1 occurrence");
        }
        update_search_matches(ed);
        textedit_find_next(ed);
    }
}

static void sel_normalize(TextEditor *ed, int *sl, int *sc, int *el, int *ec) {
    if (ed->sel.start_line < ed->sel.end_line ||
        (ed->sel.start_line == ed->sel.end_line && ed->sel.start_col <= ed->sel.end_col)) {
        *sl = ed->sel.start_line; *sc = ed->sel.start_col;
        *el = ed->sel.end_line; *ec = ed->sel.end_col;
    } else {
        *sl = ed->sel.end_line; *sc = ed->sel.end_col;
        *el = ed->sel.start_line; *ec = ed->sel.start_col;
    }
}

static char *sel_get_text(TextEditor *ed) {
    int sl, sc, el, ec;
    sel_normalize(ed, &sl, &sc, &el, &ec);
    if (sl == el) {
        int len = ec - sc;
        char *buf = xmalloc(len + 1);
        memcpy(buf, ed->lines[sl].text + sc, len);
        buf[len] = '\0';
        return buf;
    }
    size_t total = 0;
    total += ed->lines[sl].len - sc + 1;
    for (int i = sl + 1; i < el; i++) total += ed->lines[i].len + 1;
    total += ec + 1;
    char *buf = xmalloc(total + 1);
    size_t pos = 0;
    size_t chunk = ed->lines[sl].len - sc;
    memcpy(buf, ed->lines[sl].text + sc, chunk);
    pos += chunk;
    buf[pos++] = '\n';
    for (int i = sl + 1; i < el; i++) {
        memcpy(buf + pos, ed->lines[i].text, ed->lines[i].len);
        pos += ed->lines[i].len;
        buf[pos++] = '\n';
    }
    memcpy(buf + pos, ed->lines[el].text, ec);
    pos += ec;
    buf[pos] = '\0';
    return buf;
}

static void sel_delete(TextEditor *ed) {
    int sl, sc, el, ec;
    sel_normalize(ed, &sl, &sc, &el, &ec);

    char *deleted = sel_get_text(ed);
    UndoAction a = {0};
    a.type = UNDO_DELETE_BLOCK;
    a.line = sl;
    a.col = sc;
    a.text = deleted;
    a.text_len = (int)strlen(deleted);

    TextLine *first = &ed->lines[sl];
    TextLine *last = &ed->lines[el];
    line_delete_range(first, sc, (int)first->len);
    line_insert_str(first, sc, last->text, ec);

    for (int i = sl + 1; i <= el; i++) {
        free(ed->lines[sl + 1].text);
        memmove(&ed->lines[sl + 1], &ed->lines[sl + 2], sizeof(TextLine) * (ed->line_count - sl - 2));
        ed->line_count--;
    }

    ed->cursor_line = sl;
    ed->cursor_col = sc;
    ed->sel.active = false;
    ed->modified = true;
    push_undo(ed, &a);
}

static void do_paste(TextEditor *ed, const char *text) {
    UndoAction a = {0};
    a.type = UNDO_INSERT_BLOCK;
    a.line = ed->cursor_line;
    a.col = ed->cursor_col;
    a.text = xstrdup(text);
    a.text_len = (int)strlen(text);

    const char *p = text;
    const char *line_start = text;

    const char *nl = strchr(p, '\n');
    if (!nl) {
        line_insert_str(&ed->lines[ed->cursor_line], ed->cursor_col, text, (int)strlen(text));
        ed->cursor_col += (int)strlen(text);
    } else {
        TextLine *cur = &ed->lines[ed->cursor_line];
        size_t after_col = cur->len - ed->cursor_col;
        char *after = NULL;
        if (after_col > 0) {
            after = xmalloc(after_col + 1);
            memcpy(after, cur->text + ed->cursor_col, after_col);
            after[after_col] = '\0';
        }

        size_t first_len = nl - line_start;
        line_delete_range(cur, ed->cursor_col, (int)cur->len);
        line_insert_str(cur, ed->cursor_col, line_start, (int)first_len);

        p = nl + 1;
        line_start = p;
        int insert_at = ed->cursor_line + 1;

        while ((nl = strchr(p, '\n')) != NULL) {
            size_t llen = nl - line_start;
            insert_line(ed, insert_at, line_start, llen);
            ed->cursor_line = insert_at;
            insert_at++;
            p = nl + 1;
            line_start = p;
        }

        if (*line_start || after) {
            if (*line_start) {
                insert_line(ed, insert_at, line_start, strlen(line_start));
                ed->cursor_line = insert_at;
            }
            if (after) {
                TextLine *target = &ed->lines[ed->cursor_line];
                line_insert_str(target, (int)target->len, after, (int)after_col);
                ed->cursor_col = (int)target->len - (int)after_col;
                free(after);
            } else {
                ed->cursor_col = (int)strlen(line_start);
            }
        } else {
            ed->cursor_col = 0;
        }
    }

    a.line2 = ed->cursor_line;
    a.col2 = ed->cursor_col;
    ed->modified = true;
    push_undo(ed, &a);
}

static bool is_in_selection(TextEditor *ed, int line, int col) {
    if (!ed->sel.active) return false;
    int sl, sc, el, ec;
    sel_normalize(ed, &sl, &sc, &el, &ec);
    if (line < sl || line > el) return false;
    if (line == sl && col < sc) return false;
    if (line == el && col >= ec) return false;
    return true;
}

static bool is_search_match_at(TextEditor *ed, int line, int col) {
    if (!ed->search_active) return false;
    int slen = (int)strlen(ed->search_str);
    for (int i = 0; i < ed->search_match_count; i++) {
        int ml = ed->search_matches[i * 2];
        int mc = ed->search_matches[i * 2 + 1];
        if (ml == line && col >= mc && col < mc + slen) return true;
    }
    return false;
}

static bool is_bookmark_line(TextEditor *ed, int line) {
    for (int i = 0; i < ed->bookmark_count; i++) {
        if (ed->bookmarks[i] == line) return true;
    }
    return false;
}

typedef enum {
    HL_NONE = 0,
    HL_KEYWORD,
    HL_TYPE,
    HL_STRING,
    HL_COMMENT,
    HL_NUMBER,
    HL_SPECIAL,
    HL_PREPROC
} HlType;

typedef struct {
    HlType type;
    int start;
    int end;
} HlSpan;

static bool is_kw(const char **list, int count, const char *word, int wlen) {
    for (int i = 0; i < count; i++) {
        if (strlen(list[i]) == (size_t)wlen && memcmp(list[i], word, wlen) == 0)
            return true;
    }
    return false;
}

static int highlight_line(TextEditor *ed, int line_idx, HlSpan *spans, int max_spans) {
    if (ed->syntax == SYNTAX_NONE) return 0;
    SyntaxDef *sd = &syntax_defs[ed->syntax];
    TextLine *l = &ed->lines[line_idx];
    int span_count = 0;
    int i = 0;

    while (i < (int)l->len && span_count < max_spans - 4) {
        if (sd->single_line_comment[0] && strncmp(l->text + i, sd->single_line_comment, strlen(sd->single_line_comment)) == 0) {
            spans[span_count].type = HL_COMMENT;
            spans[span_count].start = i;
            spans[span_count].end = (int)l->len;
            span_count++;
            i = (int)l->len;
            continue;
        }

        if (l->text[i] == '"' || l->text[i] == '\'') {
            char quote = l->text[i];
            int start = i;
            i++;
            while (i < (int)l->len && l->text[i] != quote) {
                if (l->text[i] == '\\' && i + 1 < (int)l->len) i++;
                i++;
            }
            if (i < (int)l->len) i++;
            spans[span_count].type = HL_STRING;
            spans[span_count].start = start;
            spans[span_count].end = i;
            span_count++;
            continue;
        }

        if (isdigit((unsigned char)l->text[i])) {
            int start = i;
            while (i < (int)l->len && (isdigit((unsigned char)l->text[i]) || l->text[i] == '.' ||
                   l->text[i] == 'x' || l->text[i] == 'X' ||
                   (l->text[i] >= 'a' && l->text[i] <= 'f') ||
                   (l->text[i] >= 'A' && l->text[i] <= 'F')))
                i++;
            if (i < (int)l->len && (l->text[i] == 'u' || l->text[i] == 'U' || l->text[i] == 'l' || l->text[i] == 'L'))
                i++;
            spans[span_count].type = HL_NUMBER;
            spans[span_count].start = start;
            spans[span_count].end = i;
            span_count++;
            continue;
        }

        if (isalpha((unsigned char)l->text[i]) || l->text[i] == '_') {
            int start = i;
            while (i < (int)l->len && (isalnum((unsigned char)l->text[i]) || l->text[i] == '_'))
                i++;
            int wlen = i - start;
            if (sd->keywords && is_kw(sd->keywords, sd->keyword_count, l->text + start, wlen)) {
                spans[span_count].type = HL_KEYWORD;
            } else if (sd->types && is_kw(sd->types, sd->type_count, l->text + start, wlen)) {
                spans[span_count].type = HL_TYPE;
            } else if (ed->syntax == SYNTAX_C && l->text[start] == '#') {
                spans[span_count].type = HL_PREPROC;
            } else {
                continue;
            }
            spans[span_count].start = start;
            spans[span_count].end = i;
            span_count++;
            continue;
        }

        i++;
    }
    return span_count;
}

static int get_hl_color(HlType type) {
    switch (type) {
        case HL_KEYWORD: return COLOR_SYN_KEYWORD;
        case HL_TYPE: return COLOR_SYN_TYPE;
        case HL_STRING: return COLOR_SYN_STRING;
        case HL_COMMENT: return COLOR_SYN_COMMENT;
        case HL_NUMBER: return COLOR_SYN_NUMBER;
        case HL_PREPROC: return COLOR_SYN_SPECIAL;
        default: return COLOR_FILE;
    }
}

static int gutter_width(int total_lines) {
    int w = 3;
    if (total_lines >= 10) w = 4;
    if (total_lines >= 100) w = 5;
    if (total_lines >= 1000) w = 6;
    if (total_lines >= 10000) w = 7;
    return w + 2;
}

void textedit_draw(TextEditor *ed, WINDOW *win, int w, int h) {
    (void)win;
    ed->width = w;
    ed->height = h;

    int data_rows = h - 3;
    if (data_rows < 1) data_rows = 1;

    attron(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    mvaddstr(0, 0, "\u250f");
    mvwhline(stdscr, 0, 1, 0x2501, w - 2);
    mvaddstr(0, w - 1, "\u2513");
    attroff(COLOR_PAIR(COLOR_BORDER) | A_BOLD);

    const char *basename = strrchr(ed->path, '/');
    basename = basename ? basename + 1 : ed->path;
    char title[256];
    snprintf(title, sizeof(title), " Text Editor: %s%s ", basename, ed->modified ? " [MODIFIED]" : "");
    attron(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    mvprintw(0, 2, "%s", title);
    attroff(COLOR_PAIR(COLOR_BORDER) | A_BOLD);

    int gw = gutter_width(ed->line_count);
    int text_area_w = w - gw - 2;
    if (text_area_w < 10) text_area_w = 10;

    ensure_visible(ed, data_rows);

    int max_spans = 128;
    HlSpan *spans = NULL;
    if (ed->syntax != SYNTAX_NONE) {
        spans = alloca(sizeof(HlSpan) * max_spans);
    }

    for (int row = 0; row < data_rows; row++) {
        int data_row = ed->scroll_line + row;
        int screen_y = row + 1;
        if (screen_y >= h - 2) break;

        if (data_row >= ed->line_count) {
            attron(COLOR_PAIR(COLOR_PREVIEW));
            mvprintw(screen_y, 1, "%*d ", gw - 2, data_row + 1);
            mvhline(screen_y, gw, ' ', text_area_w);
            attroff(COLOR_PAIR(COLOR_PREVIEW));
            continue;
        }

        bool has_bookmark = is_bookmark_line(ed, data_row);
        int bm_col = has_bookmark ? 1 : 0;

        if (data_row == ed->cursor_line) {
            attron(COLOR_PAIR(COLOR_STATUS) | A_BOLD);
        } else {
            attron(COLOR_PAIR(COLOR_PREVIEW));
        }
        if (has_bookmark) {
            mvprintw(screen_y, 0, ">");
        }
        mvprintw(screen_y, bm_col, "%*d ", gw - 2 - bm_col, data_row + 1);
        if (data_row == ed->cursor_line) {
            attroff(COLOR_PAIR(COLOR_STATUS) | A_BOLD);
        } else {
            attroff(COLOR_PAIR(COLOR_PREVIEW));
        }

        TextLine *l = &ed->lines[data_row];
        int span_count = 0;
        if (spans) {
            span_count = highlight_line(ed, data_row, spans, max_spans);
        }

        int cur_col = ed->cursor_line == data_row ? ed->cursor_col : -1;
        int drawn = 0;

        for (int ci = ed->scroll_col; ci < (int)l->len && drawn < text_area_w; ci++) {
            int screen_x = gw + drawn;
            if (screen_x >= w - 1) break;

            bool is_cursor = (ci == cur_col);
            bool in_sel = is_in_selection(ed, data_row, ci);
            bool is_match = is_search_match_at(ed, data_row, ci);

            int color = COLOR_FILE;
            for (int s = 0; s < span_count; s++) {
                if (ci >= spans[s].start && ci < spans[s].end) {
                    color = get_hl_color(spans[s].type);
                    break;
                }
            }

            if (is_cursor) {
                attron(COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
                char ch = l->text[ci];
                if (ch == '\t') ch = ' ';
                mvaddch(screen_y, screen_x, isprint((unsigned char)ch) ? ch : '.');
                attroff(COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
            } else if (in_sel) {
                attron(COLOR_PAIR(20) | A_BOLD);
                char ch = l->text[ci];
                if (ch == '\t') ch = ' ';
                mvaddch(screen_y, screen_x, isprint((unsigned char)ch) ? ch : '.');
                attroff(COLOR_PAIR(20) | A_BOLD);
            } else if (is_match) {
                attron(COLOR_PAIR(COLOR_SYN_MATCH) | A_BOLD);
                char ch = l->text[ci];
                if (ch == '\t') ch = ' ';
                mvaddch(screen_y, screen_x, isprint((unsigned char)ch) ? ch : '.');
                attroff(COLOR_PAIR(COLOR_SYN_MATCH) | A_BOLD);
            } else if (color != COLOR_FILE) {
                attron(COLOR_PAIR(color));
                char ch = l->text[ci];
                if (ch == '\t') {
                    int spaces = TAB_SIZE - (drawn % TAB_SIZE);
                    for (int sp = 0; sp < spaces && drawn < text_area_w; sp++) {
                        screen_x = gw + drawn;
                        if (screen_x < w - 1) mvaddch(screen_y, screen_x, ' ');
                        drawn++;
                    }
                    attroff(COLOR_PAIR(color));
                    continue;
                }
                mvaddch(screen_y, screen_x, isprint((unsigned char)ch) ? ch : '.');
                attroff(COLOR_PAIR(color));
            } else {
                char ch = l->text[ci];
                if (ch == '\t') {
                    int spaces = TAB_SIZE - (drawn % TAB_SIZE);
                    for (int sp = 0; sp < spaces && drawn < text_area_w; sp++) {
                        screen_x = gw + drawn;
                        if (screen_x < w - 1) mvaddch(screen_y, screen_x, ' ');
                        drawn++;
                    }
                    continue;
                }
                attron(COLOR_PAIR(COLOR_FILE));
                mvaddch(screen_y, screen_x, isprint((unsigned char)ch) ? ch : '.');
                attroff(COLOR_PAIR(COLOR_FILE));
            }
            drawn++;
        }

        if (drawn < text_area_w) {
            attron(COLOR_PAIR(COLOR_FILE));
            mvhline(screen_y, gw + drawn, ' ', text_area_w - drawn);
            attroff(COLOR_PAIR(COLOR_FILE));
        }
    }

    attron(COLOR_PAIR(COLOR_BORDER));
    mvaddstr(h - 2, 0, "\u2517");
    mvwhline(stdscr, h - 2, 1, 0x2501, w - 2);
    mvaddstr(h - 2, w - 1, "\u251b");
    attroff(COLOR_PAIR(COLOR_BORDER));

    attron(COLOR_PAIR(20) | A_BOLD);
    const char *mode_str = ed->insert_mode ? " INS " : " EDIT ";
    mvprintw(h - 2, 2, "%s", mode_str);
    attroff(COLOR_PAIR(20) | A_BOLD);

    attron(COLOR_PAIR(21));
    size_t percent = ed->line_count > 0 ? ((size_t)ed->cursor_line * 100 / (size_t)ed->line_count) : 0;
    mvprintw(h - 2, 8, " L%d:C%d/%d %3zu%% ", ed->cursor_line + 1, ed->cursor_col + 1, ed->line_count, percent);
    attroff(COLOR_PAIR(21));

    attron(COLOR_PAIR(COLOR_STATUS));
    mvprintw(h - 2, 30, " F2:Save  Ctrl+S:Save  /:Find  Ctrl+G:Goto  Ctrl+Z:Undo  Esc:Quit ");
    attroff(COLOR_PAIR(COLOR_STATUS));

    attron(COLOR_PAIR(COLOR_STATUS));
    mvhline(h - 1, 0, ' ', w);
    if (ed->status_msg[0]) {
        attron(COLOR_PAIR(22) | A_BOLD);
        mvprintw(h - 1, 2, " %s ", ed->status_msg);
        attroff(COLOR_PAIR(22) | A_BOLD);
    } else if (ed->cursor_line < ed->line_count) {
        TextLine *cl = &ed->lines[ed->cursor_line];
        if (ed->cursor_col < (int)cl->len) {
            unsigned char ch = (unsigned char)cl->text[ed->cursor_col];
            attron(COLOR_PAIR(COLOR_STATUS) | A_DIM);
            mvprintw(h - 1, 2, "Char: '%c' (0x%02x %03d)  Line:%d/%d  Col:%d",
                     isprint(ch) ? ch : '.', ch, ch,
                     ed->cursor_line + 1, ed->line_count, ed->cursor_col + 1);
            attroff(COLOR_PAIR(COLOR_STATUS) | A_DIM);
        } else {
            attron(COLOR_PAIR(COLOR_STATUS) | A_DIM);
            mvprintw(h - 1, 2, "Line:%d/%d  Col:%d  EOL",
                     ed->cursor_line + 1, ed->line_count, ed->cursor_col + 1);
            attroff(COLOR_PAIR(COLOR_STATUS) | A_DIM);
        }
    }
    attroff(COLOR_PAIR(COLOR_STATUS));
}

static void do_undo(TextEditor *ed) {
    if (ed->undo_count == 0) return;
    UndoAction a = ed->undo_stack[--ed->undo_count];

    switch (a.type) {
        case UNDO_INSERT_CHAR: {
            if (a.line < ed->line_count && a.col <= (int)ed->lines[a.line].len) {
                line_delete_char(&ed->lines[a.line], a.col);
            }
            ed->cursor_line = a.line;
            ed->cursor_col = a.col;
            UndoAction r = a;
            r.text = NULL;
            push_redo(ed, &r);
            break;
        }
        case UNDO_DELETE_CHAR: {
            if (a.line < ed->line_count && a.text && a.text_len == 1) {
                line_insert_char(&ed->lines[a.line], a.col, a.text[0]);
            }
            ed->cursor_line = a.line;
            ed->cursor_col = a.col;
            UndoAction r = a;
            r.text = xstrdup(a.text);
            push_redo(ed, &r);
            break;
        }
        case UNDO_SPLIT_LINE: {
            if (a.line + 1 < ed->line_count && a.line < ed->line_count) {
                TextLine *cur = &ed->lines[a.line];
                TextLine *next = &ed->lines[a.line + 1];
                line_insert_str(cur, (int)cur->len, next->text, (int)next->len);
                delete_line(ed, a.line + 1);
            }
            ed->cursor_line = a.line;
            ed->cursor_col = a.col;
            UndoAction r = a;
            r.text = NULL;
            push_redo(ed, &r);
            break;
        }
        case UNDO_JOIN_LINE: {
            if (a.line < ed->line_count && a.text) {
                int col_pos = a.col;
                insert_line(ed, a.line + 1, "", 0);
                TextLine *cur = &ed->lines[a.line];
                TextLine *next = &ed->lines[a.line + 1];
                if (col_pos < (int)cur->len) {
                    size_t after_len = cur->len - col_pos;
                    line_ensure_cap(next, after_len + 1);
                    memcpy(next->text, cur->text + col_pos, after_len);
                    next->text[after_len] = '\0';
                    next->len = after_len;
                    line_delete_range(cur, col_pos, (int)cur->len);
                }
            }
            ed->cursor_line = a.line;
            ed->cursor_col = a.col;
            UndoAction r = a;
            r.text = NULL;
            push_redo(ed, &r);
            break;
        }
        case UNDO_INSERT_BLOCK: {
            if (a.text) {
                int sl = a.line;
                int sc = a.col;
                int el = a.line2;
                int ec = a.line2;
                TextLine *first = &ed->lines[sl];

                const char *p = a.text;
                const char *nl = strchr(p, '\n');
                if (!nl) {
                    line_delete_range(first, sc, sc + (int)strlen(a.text));
                } else {
                    size_t first_len = nl - p;
                    line_delete_range(first, sc, sc + (int)first_len);
                    const char *line_start = nl + 1;
                    int del_line = sl + 1;
                    while ((nl = strchr(line_start, '\n')) != NULL) {
                        if (del_line < ed->line_count) {
                            free(ed->lines[del_line].text);
                            memmove(&ed->lines[del_line], &ed->lines[del_line + 1],
                                    sizeof(TextLine) * (ed->line_count - del_line - 1));
                            ed->line_count--;
                        }
                        line_start = nl + 1;
                    }
                    if (*line_start && del_line < ed->line_count) {
                        free(ed->lines[del_line].text);
                        memmove(&ed->lines[del_line], &ed->lines[del_line + 1],
                                sizeof(TextLine) * (ed->line_count - del_line - 1));
                        ed->line_count--;
                    }
                    if (ec > 0 && el < ed->line_count && el != sl) {
                        line_delete_range(&ed->lines[sl + 1], 0, ec);
                    }
                }
            }
            ed->cursor_line = a.line;
            ed->cursor_col = a.col;
            UndoAction r = a;
            r.text = xstrdup(a.text);
            push_redo(ed, &r);
            break;
        }
        case UNDO_DELETE_BLOCK: {
            if (a.text) {
                ed->cursor_line = a.line;
                ed->cursor_col = a.col;
                do_paste(ed, a.text);
                ed->cursor_line = a.line;
                ed->cursor_col = a.col;
            }
            UndoAction r = a;
            r.text = xstrdup(a.text);
            push_redo(ed, &r);
            break;
        }
        case UNDO_REPLACE:
            break;
    }
    free(a.text);
    ed->modified = true;
    update_search_matches(ed);
}

static void do_redo(TextEditor *ed) {
    if (ed->redo_count == 0) return;
    UndoAction a = ed->redo_stack[--ed->redo_count];

    switch (a.type) {
        case UNDO_INSERT_CHAR: {
            if (a.line < ed->line_count && a.text && a.text_len == 1) {
                line_insert_char(&ed->lines[a.line], a.col, a.text[0]);
            }
            ed->cursor_line = a.line;
            ed->cursor_col = a.col + 1;
            break;
        }
        case UNDO_DELETE_CHAR: {
            if (a.line < ed->line_count && a.col <= (int)ed->lines[a.line].len) {
                line_delete_char(&ed->lines[a.line], a.col);
            }
            ed->cursor_line = a.line;
            ed->cursor_col = a.col;
            break;
        }
        case UNDO_SPLIT_LINE: {
            if (a.line < ed->line_count) {
                TextLine *cur = &ed->lines[a.line];
                int col_pos = a.col;
                if (col_pos <= (int)cur->len) {
                    size_t after_len = cur->len - col_pos;
                    insert_line(ed, a.line + 1, cur->text + col_pos, after_len);
                    line_delete_range(cur, col_pos, (int)cur->len);
                }
            }
            ed->cursor_line = a.line + 1;
            ed->cursor_col = 0;
            break;
        }
        case UNDO_JOIN_LINE: {
            if (a.line + 1 < ed->line_count) {
                TextLine *cur = &ed->lines[a.line];
                TextLine *next = &ed->lines[a.line + 1];
                line_insert_str(cur, (int)cur->len, next->text, (int)next->len);
                delete_line(ed, a.line + 1);
            }
            ed->cursor_line = a.line;
            ed->cursor_col = a.col;
            break;
        }
        default:
            break;
    }
    free(a.text);
    ed->modified = true;
    update_search_matches(ed);
}

static void insert_char(TextEditor *ed, char ch) {
    TextLine *l = &ed->lines[ed->cursor_line];
    UndoAction a = {0};
    a.type = UNDO_INSERT_CHAR;
    a.line = ed->cursor_line;
    a.col = ed->cursor_col;
    a.text = xmalloc(2);
    a.text[0] = ch;
    a.text[1] = '\0';
    a.text_len = 1;

    if (ed->insert_mode) {
        if (ed->cursor_col <= (int)l->len) {
            line_insert_char(l, ed->cursor_col, ch);
        }
    } else {
        if (ed->cursor_col < (int)l->len) {
            l->text[ed->cursor_col] = ch;
        } else {
            line_insert_char(l, ed->cursor_col, ch);
        }
    }
    ed->cursor_col++;
    ed->modified = true;
    push_undo(ed, &a);
}

static void do_enter(TextEditor *ed) {
    TextLine *l = &ed->lines[ed->cursor_line];
    UndoAction a = {0};
    a.type = UNDO_SPLIT_LINE;
    a.line = ed->cursor_line;
    a.col = ed->cursor_col;

    size_t after_len = l->len - ed->cursor_col;
    char *after = NULL;
    if (after_len > 0) {
        after = xmalloc(after_len + 1);
        memcpy(after, l->text + ed->cursor_col, after_len);
        after[after_len] = '\0';
    }
    line_delete_range(l, ed->cursor_col, (int)l->len);

    int indent = get_indent(ed->lines[ed->cursor_line].text, ed->lines[ed->cursor_line].len);
    char indent_buf[256];
    int indent_len = 0;
    for (int i = 0; i < indent && indent_len < 255; i++) {
        indent_buf[indent_len++] = ' ';
    }

    insert_line(ed, ed->cursor_line + 1, "", 0);
    TextLine *new_line = &ed->lines[ed->cursor_line + 1];
    if (indent_len > 0) {
        line_insert_str(new_line, 0, indent_buf, indent_len);
    }
    if (after) {
        line_insert_str(new_line, (int)new_line->len, after, (int)after_len);
        free(after);
    }

    ed->cursor_line++;
    ed->cursor_col = indent_len;
    ed->modified = true;
    push_undo(ed, &a);
}

static void do_backspace(TextEditor *ed) {
    if (ed->cursor_col > 0) {
        TextLine *l = &ed->lines[ed->cursor_line];
        UndoAction a = {0};
        a.type = UNDO_DELETE_CHAR;
        a.line = ed->cursor_line;
        a.col = ed->cursor_col - 1;
        a.text = xmalloc(2);
        a.text[0] = l->text[ed->cursor_col - 1];
        a.text[1] = '\0';
        a.text_len = 1;
        line_delete_char(l, ed->cursor_col - 1);
        ed->cursor_col--;
        ed->modified = true;
        push_undo(ed, &a);
    } else if (ed->cursor_line > 0) {
        TextLine *prev = &ed->lines[ed->cursor_line - 1];
        TextLine *cur = &ed->lines[ed->cursor_line];
        UndoAction a = {0};
        a.type = UNDO_JOIN_LINE;
        a.line = ed->cursor_line - 1;
        a.col = (int)prev->len;
        int join_col = (int)prev->len;
        line_insert_str(prev, (int)prev->len, cur->text, (int)cur->len);
        delete_line(ed, ed->cursor_line);
        ed->cursor_line--;
        ed->cursor_col = join_col;
        ed->modified = true;
        push_undo(ed, &a);
    }
}

static void do_delete(TextEditor *ed) {
    TextLine *l = &ed->lines[ed->cursor_line];
    if (ed->cursor_col < (int)l->len) {
        UndoAction a = {0};
        a.type = UNDO_DELETE_CHAR;
        a.line = ed->cursor_line;
        a.col = ed->cursor_col;
        a.text = xmalloc(2);
        a.text[0] = l->text[ed->cursor_col];
        a.text[1] = '\0';
        a.text_len = 1;
        line_delete_char(l, ed->cursor_col);
        ed->modified = true;
        push_undo(ed, &a);
    } else if (ed->cursor_line < ed->line_count - 1) {
        TextLine *next = &ed->lines[ed->cursor_line + 1];
        UndoAction a = {0};
        a.type = UNDO_JOIN_LINE;
        a.line = ed->cursor_line;
        a.col = (int)l->len;
        line_insert_str(l, (int)l->len, next->text, (int)next->len);
        delete_line(ed, ed->cursor_line + 1);
        ed->modified = true;
        push_undo(ed, &a);
    }
}

static void do_tab(TextEditor *ed) {
    UndoAction a = {0};
    a.type = UNDO_INSERT_BLOCK;
    a.line = ed->cursor_line;
    a.col = ed->cursor_col;
    char spaces[TAB_SIZE + 1];
    memset(spaces, ' ', TAB_SIZE);
    spaces[TAB_SIZE] = '\0';
    a.text = xstrdup(spaces);
    a.text_len = TAB_SIZE;
    line_insert_str(&ed->lines[ed->cursor_line], ed->cursor_col, spaces, TAB_SIZE);
    ed->cursor_col += TAB_SIZE;
    ed->modified = true;
    push_undo(ed, &a);
}

static void do_shift_tab(TextEditor *ed) {
    TextLine *l = &ed->lines[ed->cursor_line];
    int col = ed->cursor_col;
    if (col == 0) return;
    int to_dedent = 0;
    if (col >= TAB_SIZE) to_dedent = TAB_SIZE;
    else to_dedent = col;
    if (to_dedent == 0) return;
    for (int i = 0; i < to_dedent; i++) {
        if (l->text[col - to_dedent + i] != ' ') return;
    }
    line_delete_range(l, col - to_dedent, col);
    ed->cursor_col -= to_dedent;
    ed->modified = true;
}

static void do_delete_line(TextEditor *ed) {
    if (ed->line_count <= 1) {
        UndoAction a = {0};
        a.type = UNDO_DELETE_BLOCK;
        a.line = 0;
        a.col = 0;
        a.text = xstrdup(ed->lines[0].text);
        a.text_len = (int)ed->lines[0].len;
        ed->lines[0].text[0] = '\0';
        ed->lines[0].len = 0;
        ed->cursor_col = 0;
        ed->modified = true;
        push_undo(ed, &a);
        return;
    }
    UndoAction a = {0};
    a.type = UNDO_DELETE_BLOCK;
    a.line = ed->cursor_line;
    a.col = 0;
    a.text = xstrdup(ed->lines[ed->cursor_line].text);
    a.text_len = (int)ed->lines[ed->cursor_line].len;
    a.line2 = ed->cursor_line + 1;
    delete_line(ed, ed->cursor_line);
    if (ed->cursor_line >= ed->line_count) ed->cursor_line = ed->line_count - 1;
    ed->cursor_col = 0;
    ed->modified = true;
    push_undo(ed, &a);
}

static void do_duplicate_line(TextEditor *ed) {
    TextLine *l = &ed->lines[ed->cursor_line];
    insert_line(ed, ed->cursor_line + 1, l->text, l->len);
    ed->cursor_line++;
    ed->modified = true;
}

static void toggle_bookmark(TextEditor *ed) {
    for (int i = 0; i < ed->bookmark_count; i++) {
        if (ed->bookmarks[i] == ed->cursor_line) {
            ed->bookmarks[i] = ed->bookmarks[ed->bookmark_count - 1];
            ed->bookmark_count--;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Bookmark removed");
            return;
        }
    }
    if (ed->bookmark_count < MAX_BOOKMARKS) {
        ed->bookmarks[ed->bookmark_count++] = ed->cursor_line;
        snprintf(ed->status_msg, sizeof(ed->status_msg), "Bookmark set at line %d", ed->cursor_line + 1);
    }
}

static void next_bookmark(TextEditor *ed) {
    if (ed->bookmark_count == 0) return;
    int best = -1;
    for (int i = 0; i < ed->bookmark_count; i++) {
        if (ed->bookmarks[i] > ed->cursor_line) {
            if (best == -1 || ed->bookmarks[i] < ed->bookmarks[best]) best = i;
        }
    }
    if (best == -1) best = 0;
    ed->cursor_line = ed->bookmarks[best];
    ed->cursor_col = 0;
    snprintf(ed->status_msg, sizeof(ed->status_msg), "Bookmark: line %d", ed->cursor_line + 1);
}

static void prev_bookmark(TextEditor *ed) {
    if (ed->bookmark_count == 0) return;
    int best = -1;
    for (int i = 0; i < ed->bookmark_count; i++) {
        if (ed->bookmarks[i] < ed->cursor_line) {
            if (best == -1 || ed->bookmarks[i] > ed->bookmarks[best]) best = i;
        }
    }
    if (best == -1) best = ed->bookmark_count - 1;
    ed->cursor_line = ed->bookmarks[best];
    ed->cursor_col = 0;
    snprintf(ed->status_msg, sizeof(ed->status_msg), "Bookmark: line %d", ed->cursor_line + 1);
}

void textedit_handle_key(TextEditor *ed, int key) {
    ed->status_msg[0] = '\0';
    int data_rows = ed->height - 3;
    if (data_rows < 1) data_rows = 1;

    if (ed->sel.active && !(key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT ||
        key == KEY_RIGHT || key == KEY_HOME || key == KEY_END || key == KEY_PPAGE || key == KEY_NPAGE)) {
        ed->sel.active = false;
    }

    switch (key) {
        case KEY_UP:
            if (ed->cursor_line > 0) {
                ed->cursor_line--;
                int new_col = 0;
                int tab_col = 0;
                TextLine *l = &ed->lines[ed->cursor_line];
                while (new_col < (int)l->len && tab_col < ed->cursor_col) {
                    if (l->text[new_col] == '\t') tab_col += TAB_SIZE - (tab_col % TAB_SIZE);
                    else tab_col++;
                    new_col++;
                }
                ed->cursor_col = new_col;
            }
            break;
        case KEY_DOWN:
            if (ed->cursor_line < ed->line_count - 1) {
                ed->cursor_line++;
                int new_col = 0;
                int tab_col = 0;
                TextLine *l = &ed->lines[ed->cursor_line];
                while (new_col < (int)l->len && tab_col < ed->cursor_col) {
                    if (l->text[new_col] == '\t') tab_col += TAB_SIZE - (tab_col % TAB_SIZE);
                    else tab_col++;
                    new_col++;
                }
                ed->cursor_col = new_col;
            }
            break;
        case KEY_LEFT:
            if (ed->cursor_col > 0) {
                ed->cursor_col--;
            } else if (ed->cursor_line > 0) {
                ed->cursor_line--;
                ed->cursor_col = (int)ed->lines[ed->cursor_line].len;
            }
            break;
        case KEY_RIGHT:
            if (ed->cursor_col < (int)ed->lines[ed->cursor_line].len) {
                ed->cursor_col++;
            } else if (ed->cursor_line < ed->line_count - 1) {
                ed->cursor_line++;
                ed->cursor_col = 0;
            }
            break;
        case KEY_HOME:
            ed->cursor_col = 0;
            break;
        case KEY_END:
            ed->cursor_col = (int)ed->lines[ed->cursor_line].len;
            break;
        case 535:  // Ctrl+Home (some terminals)
        case 557:  // Ctrl+Home (another variant)
            ed->cursor_line = 0;
            ed->cursor_col = 0;
            ed->scroll_line = 0;
            break;
        case 532:  // Ctrl+End
        case 554:
            ed->cursor_line = ed->line_count - 1;
            ed->cursor_col = (int)ed->lines[ed->cursor_line].len;
            break;
        case KEY_PPAGE: {
            int page = data_rows;
            if (ed->cursor_line > page) ed->cursor_line -= page;
            else ed->cursor_line = 0;
            ed->cursor_col = 0;
            break;
        }
        case KEY_NPAGE: {
            int page = data_rows;
            if (ed->cursor_line + page < ed->line_count) ed->cursor_line += page;
            else ed->cursor_line = ed->line_count - 1;
            ed->cursor_col = 0;
            break;
        }
        case KEY_IC:
            ed->insert_mode = !ed->insert_mode;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Mode: %s", ed->insert_mode ? "Insert" : "Overwrite");
            break;
        case KEY_F(2):
            textedit_save(ed);
            break;
        case 19:  // Ctrl+S
            textedit_save(ed);
            break;
        case 26:  // Ctrl+Z
            do_undo(ed);
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Undo (%d remaining)", ed->undo_count);
            break;
        case 25:  // Ctrl+Y
            do_redo(ed);
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Redo (%d remaining)", ed->redo_count);
            break;
        case 7: {  // Ctrl+G - goto line
            char prompt[64];
            snprintf(prompt, sizeof(prompt), "Go to line (1-%d):", ed->line_count);
            char *input = ui_input(prompt, "");
            if (input) {
                int line = atoi(input);
                textedit_goto_line(ed, line);
                free(input);
            }
            break;
        }
        case '/': {
            char *input = ui_input("Find:", ed->search_str);
            if (input) {
                textedit_find(ed, input);
                free(input);
            }
            break;
        }
        case 'n':
            textedit_find_next(ed);
            break;
        case 'N':
            textedit_find_prev(ed);
            break;
        case 8: {  // Ctrl+H - search and replace
            char *find = ui_input("Find:", ed->search_str);
            if (find) {
                char *replace = ui_input("Replace with:", "");
                if (replace) {
                    textedit_replace(ed, find, replace, false);
                    free(replace);
                }
                free(find);
            }
            break;
        }
        case 23:  // Ctrl+W - word wrap toggle
            ed->word_wrap = !ed->word_wrap;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Word wrap: %s", ed->word_wrap ? "ON" : "OFF");
            break;
        case 2:  // Ctrl+B - toggle bookmark
            toggle_bookmark(ed);
            break;
        case 14:  // Ctrl+N - next bookmark
            next_bookmark(ed);
            break;
        case 16:  // Ctrl+P - prev bookmark
            prev_bookmark(ed);
            break;
        case 11:  // Ctrl+K - delete line
            do_delete_line(ed);
            break;
        case 4:  // Ctrl+D - duplicate line
            do_duplicate_line(ed);
            break;
        case 1: {  // Ctrl+A - select all
            ed->sel.active = true;
            ed->sel.start_line = 0;
            ed->sel.start_col = 0;
            ed->sel.end_line = ed->line_count - 1;
            ed->sel.end_col = (int)ed->lines[ed->line_count - 1].len;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Selected all");
            break;
        }
        case 3: {  // Ctrl+C - copy selection
            if (ed->sel.active) {
                char *text = sel_get_text(ed);
                clipboard_copy_text(text);
                snprintf(ed->status_msg, sizeof(ed->status_msg), "Copied to clipboard");
                free(text);
            }
            break;
        }
        case 24: {  // Ctrl+X - cut selection
            if (ed->sel.active) {
                char *text = sel_get_text(ed);
                clipboard_copy_text(text);
                free(text);
                sel_delete(ed);
                snprintf(ed->status_msg, sizeof(ed->status_msg), "Cut to clipboard");
            }
            break;
        }
        case 22: {  // Ctrl+V - paste
            char *text = clipboard_paste_text();
            if (text) {
                if (ed->sel.active) sel_delete(ed);
                do_paste(ed, text);
                free(text);
                snprintf(ed->status_msg, sizeof(ed->status_msg), "Pasted");
            }
            break;
        }
        case KEY_BACKSPACE:
        case 127:
            if (ed->sel.active) {
                sel_delete(ed);
            } else {
                do_backspace(ed);
            }
            break;
        case KEY_DC:
            if (ed->sel.active) {
                sel_delete(ed);
            } else {
                do_delete(ed);
            }
            break;
        case '\n':
        case KEY_ENTER:
            if (ed->sel.active) sel_delete(ed);
            do_enter(ed);
            break;
        case '\t':
            do_tab(ed);
            break;
        case 353:  // Shift+Tab
            do_shift_tab(ed);
            break;
        case KEY_SLEFT:
            if (!ed->sel.active) {
                ed->sel.active = true;
                ed->sel.start_line = ed->cursor_line;
                ed->sel.start_col = ed->cursor_col;
            }
            if (ed->cursor_col > 0) ed->cursor_col--;
            else if (ed->cursor_line > 0) {
                ed->cursor_line--;
                ed->cursor_col = (int)ed->lines[ed->cursor_line].len;
            }
            ed->sel.end_line = ed->cursor_line;
            ed->sel.end_col = ed->cursor_col;
            break;
        case KEY_SRIGHT:
            if (!ed->sel.active) {
                ed->sel.active = true;
                ed->sel.start_line = ed->cursor_line;
                ed->sel.start_col = ed->cursor_col;
            }
            if (ed->cursor_col < (int)ed->lines[ed->cursor_line].len) ed->cursor_col++;
            else if (ed->cursor_line < ed->line_count - 1) {
                ed->cursor_line++;
                ed->cursor_col = 0;
            }
            ed->sel.end_line = ed->cursor_line;
            ed->sel.end_col = ed->cursor_col;
            break;
        case 337:  // Shift+Up (KEY_SUP alternative)
            if (!ed->sel.active) {
                ed->sel.active = true;
                ed->sel.start_line = ed->cursor_line;
                ed->sel.start_col = ed->cursor_col;
            }
            if (ed->cursor_line > 0) ed->cursor_line--;
            ed->sel.end_line = ed->cursor_line;
            ed->sel.end_col = ed->cursor_col;
            break;
        case 336:  // Shift+Down (KEY_SDOWN alternative)
            if (!ed->sel.active) {
                ed->sel.active = true;
                ed->sel.start_line = ed->cursor_line;
                ed->sel.start_col = ed->cursor_col;
            }
            if (ed->cursor_line < ed->line_count - 1) ed->cursor_line++;
            ed->sel.end_line = ed->cursor_line;
            ed->sel.end_col = ed->cursor_col;
            break;
        default:
            if (key >= 32 && key < 127) {
                if (ed->sel.active) sel_delete(ed);
                insert_char(ed, (char)key);
            }
            break;
    }

    clamp_cursor(ed);
    ensure_visible(ed, data_rows);
}
