#include "hexedit.h"
#include "platform.h"
#include "ui.h"
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#define BYTES_PER_ROW 16
#define ADDR_WIDTH 8
#define HEX_GAP 2
#define ASCII_GAP 2
#define HEX_COL_WIDTH 3
#define ASCII_COLS 16

static int hex_to_val(int ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static void ensure_capacity(HexEditor *ed, size_t needed) {
    if (needed <= ed->capacity) return;
    size_t new_cap = ed->capacity ? ed->capacity : 4096;
    while (new_cap < needed) new_cap *= 2;
    ed->data = xrealloc(ed->data, new_cap);
    if (ed->file_size < new_cap) {
        memset(ed->data + ed->file_size, 0, new_cap - ed->file_size);
    }
    ed->capacity = new_cap;
}

static void clamp_cursor(HexEditor *ed) {
    if (ed->cursor > ed->file_size) ed->cursor = ed->file_size;
    if (ed->insert_mode) {
        if (ed->cursor > ed->file_size) ed->cursor = ed->file_size;
    } else {
        if (ed->cursor >= ed->file_size && ed->file_size > 0) ed->cursor = ed->file_size - 1;
    }
}

static void ensure_visible(HexEditor *ed, int visible_rows) {
    int cursor_row = (int)(ed->cursor / BYTES_PER_ROW);
    if (cursor_row < ed->scroll_offset) {
        ed->scroll_offset = cursor_row;
    }
    if (cursor_row >= ed->scroll_offset + visible_rows) {
        ed->scroll_offset = cursor_row - visible_rows + 1;
    }
    if (ed->scroll_offset < 0) ed->scroll_offset = 0;
}

int hexedit_open(HexEditor *ed, const char *path) {
    memset(ed, 0, sizeof(HexEditor));
    strncpy(ed->path, path, MAX_PATH - 1);

    size_t len;
    char *content = pl_read_file(path, SIZE_MAX, &len);
    if (!content) {
        ed->data = xmalloc(4096);
        ed->capacity = 4096;
        ed->file_size = 0;
        memset(ed->data, 0, ed->capacity);
    } else {
        ed->file_size = len;
        ed->capacity = len + 1;
        ed->data = (unsigned char *)content;
    }

    ed->cursor = 0;
    ed->scroll_offset = 0;
    ed->modified = false;
    ed->hex_nibble = -1;
    ed->editing_ascii = false;
    ed->insert_mode = false;
    ed->status_msg[0] = '\0';
    return 0;
}

void hexedit_close(HexEditor *ed) {
    if (ed->data) {
        free(ed->data);
        ed->data = NULL;
    }
    ed->file_size = 0;
    ed->capacity = 0;
}

int hexedit_save(HexEditor *ed) {
    FILE *f = fopen(ed->path, "wb");
    if (!f) {
        snprintf(ed->status_msg, sizeof(ed->status_msg), "Error: cannot open file for writing");
        return -1;
    }
    size_t written = fwrite(ed->data, 1, ed->file_size, f);
    fclose(f);
    if (written != ed->file_size) {
        snprintf(ed->status_msg, sizeof(ed->status_msg), "Error: write incomplete");
        return -1;
    }
    ed->modified = false;
    snprintf(ed->status_msg, sizeof(ed->status_msg), "Saved %zu bytes", ed->file_size);
    return 0;
}

int hexedit_save_as(HexEditor *ed, const char *path) {
    char old[MAX_PATH];
    strncpy(old, ed->path, MAX_PATH - 1);
    strncpy(ed->path, path, MAX_PATH - 1);
    int ret = hexedit_save(ed);
    if (ret != 0) {
        strncpy(ed->path, old, MAX_PATH - 1);
    }
    return ret;
}

void hexedit_goto(HexEditor *ed, size_t offset) {
    if (offset > ed->file_size) offset = ed->file_size;
    ed->cursor = offset;
    ed->status_msg[0] = '\0';
}

void hexedit_find(HexEditor *ed, const unsigned char *pattern, size_t pat_len) {
    if (pat_len == 0 || pat_len > ed->file_size) return;
    for (size_t i = ed->cursor + 1; i <= ed->file_size - pat_len; i++) {
        if (memcmp(ed->data + i, pattern, pat_len) == 0) {
            ed->cursor = i;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Found at 0x%08zx", i);
            return;
        }
    }
    snprintf(ed->status_msg, sizeof(ed->status_msg), "Pattern not found");
}

void hexedit_draw(HexEditor *ed, WINDOW *win, int w, int h) {
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
    snprintf(title, sizeof(title), " Hex Editor: %s%s ", basename, ed->modified ? " [MODIFIED]" : "");
    attron(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    mvprintw(0, 2, "%s", title);
    attroff(COLOR_PAIR(COLOR_BORDER) | A_BOLD);

    attron(COLOR_PAIR(21));
    mvprintw(0, w - 30, " Offset    00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F ");
    attroff(COLOR_PAIR(21));

    int cursor_row = (int)(ed->cursor / BYTES_PER_ROW);
    int cursor_col = (int)(ed->cursor % BYTES_PER_ROW);
    ensure_visible(ed, data_rows);

    for (int row = 0; row < data_rows; row++) {
        int data_row = ed->scroll_offset + row;
        size_t row_offset = (size_t)data_row * BYTES_PER_ROW;
        int screen_y = row + 1;

        if (screen_y >= h - 2) break;

        attron(COLOR_PAIR(COLOR_STATUS));
        mvprintw(screen_y, 1, "%08zx", row_offset);

        int hex_start_x = ADDR_WIDTH + 2;

        for (int col = 0; col < BYTES_PER_ROW; col++) {
            size_t byte_idx = row_offset + col;
            int x = hex_start_x + col * HEX_COL_WIDTH;
            if (col >= 8) x += 1;

            if (byte_idx < ed->file_size) {
                bool is_cursor = ((int)(byte_idx) == (int)(ed->cursor)) && !ed->editing_ascii;

                if (is_cursor) {
                    if (ed->hex_nibble >= 0) {
                        attron(COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
                        mvprintw(screen_y, x, "%1X", ed->hex_nibble);
                        attron(COLOR_PAIR(20) | A_BOLD);
                        mvprintw(screen_y, x + 1, "%02x", ed->data[byte_idx] & 0x0F);
                        attroff(COLOR_PAIR(20) | A_BOLD);
                    } else {
                        attron(COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
                        mvprintw(screen_y, x, "%02x", ed->data[byte_idx]);
                        attroff(COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
                    }
                } else {
                    attron(COLOR_PAIR(COLOR_PREVIEW));
                    mvprintw(screen_y, x, "%02x", ed->data[byte_idx]);
                    attroff(COLOR_PAIR(COLOR_PREVIEW));
                }
            } else {
                attron(COLOR_PAIR(8));
                mvprintw(screen_y, x, "  ");
                attroff(COLOR_PAIR(8));
            }
        }

        int ascii_x = hex_start_x + BYTES_PER_ROW * HEX_COL_WIDTH + 2;
        if (ascii_x + ASCII_COLS + 2 <= w) {
            attron(COLOR_PAIR(COLOR_STATUS));
            mvaddstr(screen_y, ascii_x, " \u2502");
            attroff(COLOR_PAIR(COLOR_STATUS));

            for (int col = 0; col < BYTES_PER_ROW; col++) {
                size_t byte_idx = row_offset + col;
                int ch_x = ascii_x + 2 + col;

                if (byte_idx < ed->file_size) {
                    unsigned char ch = ed->data[byte_idx];
                    bool is_cursor = ((int)(byte_idx) == (int)(ed->cursor)) && ed->editing_ascii;

                    if (is_cursor) {
                        attron(COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
                        mvaddch(screen_y, ch_x, isprint(ch) ? ch : '.');
                        attroff(COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
                    } else {
                        attron(COLOR_PAIR(COLOR_FILE));
                        mvaddch(screen_y, ch_x, isprint(ch) ? ch : '.');
                        attroff(COLOR_PAIR(COLOR_FILE));
                    }
                } else {
                    mvaddch(screen_y, ch_x, ' ');
                }
            }

            attron(COLOR_PAIR(COLOR_STATUS));
            mvaddstr(screen_y, ascii_x + 2 + BYTES_PER_ROW, "\u2502");
            attroff(COLOR_PAIR(COLOR_STATUS));
        }
    }

    attron(COLOR_PAIR(COLOR_BORDER));
    mvaddstr(h - 2, 0, "\u2517");
    mvwhline(stdscr, h - 2, 1, 0x2501, w - 2);
    mvaddstr(h - 2, w - 1, "\u251b");
    attroff(COLOR_PAIR(COLOR_BORDER));

    attron(COLOR_PAIR(20) | A_BOLD);
    const char *mode_str = ed->insert_mode ? " INS " : " HEX ";
    if (ed->editing_ascii) mode_str = " ASC ";
    mvprintw(h - 2, 2, "%s", mode_str);
    attroff(COLOR_PAIR(20) | A_BOLD);

    attron(COLOR_PAIR(21));
    size_t percent = ed->file_size > 0 ? (ed->cursor * 100 / ed->file_size) : 0;
    mvprintw(h - 2, 8, " 0x%08zx/%08zx %3zu%% ", ed->cursor, ed->file_size, percent);
    attroff(COLOR_PAIR(21));

    int hint_x = 8 + 35;
    attron(COLOR_PAIR(COLOR_STATUS));
    mvprintw(h - 2, hint_x, " F2:Save  Tab:Hex/Asc  Ins:Insert  g:Goto  /:Find  Esc:Quit ");
    attroff(COLOR_PAIR(COLOR_STATUS));

    attron(COLOR_PAIR(COLOR_STATUS));
    mvhline(h - 1, 0, ' ', w);
    if (ed->status_msg[0]) {
        attron(COLOR_PAIR(22) | A_BOLD);
        mvprintw(h - 1, 2, " %s ", ed->status_msg);
        attroff(COLOR_PAIR(22) | A_BOLD);
    } else {
        attron(COLOR_PAIR(COLOR_STATUS) | A_DIM);
        mvprintw(h - 1, 2, "Byte: 0x%02x (%03d) '%c'  Row:%d Col:%d",
                 ed->cursor < ed->file_size ? ed->data[ed->cursor] : 0,
                 ed->cursor < ed->file_size ? ed->data[ed->cursor] : 0,
                 (ed->cursor < ed->file_size && isprint(ed->data[ed->cursor])) ? ed->data[ed->cursor] : '.',
                 cursor_row, cursor_col);
        attroff(COLOR_PAIR(COLOR_STATUS) | A_DIM);
    }
    attroff(COLOR_PAIR(COLOR_STATUS));

    (void)cursor_col;
}

void hexedit_handle_key(HexEditor *ed, int key) {
    ed->status_msg[0] = '\0';

    int data_rows = ed->height - 3;
    if (data_rows < 1) data_rows = 1;

    if (ed->hex_nibble >= 0) {
        int val = hex_to_val(key);
        if (val >= 0) {
            unsigned char new_val = (ed->hex_nibble << 4) | val;
            if (ed->cursor < ed->file_size) {
                ed->data[ed->cursor] = new_val;
                ed->modified = true;
            }
            ed->hex_nibble = -1;
            if (ed->cursor < ed->file_size - 1) ed->cursor++;
            return;
        }
        ed->hex_nibble = -1;
        return;
    }

    switch (key) {
        case KEY_UP:
            if (ed->cursor >= BYTES_PER_ROW) ed->cursor -= BYTES_PER_ROW;
            break;
        case KEY_DOWN:
            if (ed->cursor + BYTES_PER_ROW < ed->file_size) ed->cursor += BYTES_PER_ROW;
            break;
        case KEY_LEFT:
            if (ed->cursor > 0) ed->cursor--;
            break;
        case KEY_RIGHT:
            if (ed->cursor < ed->file_size - 1) ed->cursor++;
            break;
        case KEY_HOME:
            ed->cursor -= (ed->cursor % BYTES_PER_ROW);
            break;
        case KEY_END:
            ed->cursor -= (ed->cursor % BYTES_PER_ROW);
            ed->cursor += BYTES_PER_ROW - 1;
            if (ed->cursor >= ed->file_size) ed->cursor = ed->file_size - 1;
            break;
        case KEY_PPAGE: {
            size_t page = (size_t)data_rows * BYTES_PER_ROW;
            if (ed->cursor > page) ed->cursor -= page;
            else ed->cursor = 0;
            break;
        }
        case KEY_NPAGE: {
            size_t page = (size_t)data_rows * BYTES_PER_ROW;
            if (ed->cursor + page < ed->file_size) ed->cursor += page;
            else ed->cursor = ed->file_size - 1;
            break;
        }
        case KEY_IC:
            ed->insert_mode = !ed->insert_mode;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Mode: %s", ed->insert_mode ? "Insert" : "Overwrite");
            break;
        case '\t':
            ed->editing_ascii = !ed->editing_ascii;
            snprintf(ed->status_msg, sizeof(ed->status_msg), "Editing: %s", ed->editing_ascii ? "ASCII" : "Hex");
            break;
        case KEY_F(2):
            hexedit_save(ed);
            break;
        case 'g': {
            char *input = ui_input("Go to offset (hex):", "0");
            if (input) {
                size_t offset = strtoull(input, NULL, 16);
                hexedit_goto(ed, offset);
                free(input);
            }
            break;
        }
        case '/': {
            char *input = ui_input("Find (hex bytes, e.g. 48656C6C):", "");
            if (input) {
                size_t inlen = strlen(input);
                size_t pat_len = inlen / 2;
                if (pat_len > 0) {
                    unsigned char *pattern = xmalloc(pat_len);
                    for (size_t i = 0; i < pat_len; i++) {
                        unsigned int val;
                        sscanf(input + i * 2, "%2x", &val);
                        pattern[i] = (unsigned char)val;
                    }
                    hexedit_find(ed, pattern, pat_len);
                    free(pattern);
                }
                free(input);
            }
            break;
        }
        case KEY_BACKSPACE: case 127:
            if (ed->cursor > 0) {
                ed->cursor--;
                if (ed->insert_mode && ed->file_size > 0) {
                    memmove(ed->data + ed->cursor, ed->data + ed->cursor + 1, ed->file_size - ed->cursor - 1);
                    ed->file_size--;
                    ed->modified = true;
                }
            }
            break;
        case KEY_DC:
            if (ed->insert_mode && ed->cursor < ed->file_size && ed->file_size > 0) {
                memmove(ed->data + ed->cursor, ed->data + ed->cursor + 1, ed->file_size - ed->cursor - 1);
                ed->file_size--;
                ed->modified = true;
            }
            break;
        default:
            if (ed->editing_ascii) {
                if (key >= 32 && key < 127) {
                    if (ed->insert_mode) {
                        ensure_capacity(ed, ed->file_size + 1);
                        memmove(ed->data + ed->cursor + 1, ed->data + ed->cursor, ed->file_size - ed->cursor);
                        ed->data[ed->cursor] = (unsigned char)key;
                        ed->file_size++;
                        ed->modified = true;
                        if (ed->cursor < ed->file_size - 1) ed->cursor++;
                    } else {
                        if (ed->cursor < ed->file_size) {
                            ed->data[ed->cursor] = (unsigned char)key;
                            ed->modified = true;
                            if (ed->cursor < ed->file_size - 1) ed->cursor++;
                        }
                    }
                }
            } else {
                int val = hex_to_val(key);
                if (val >= 0) {
                    if (ed->cursor < ed->file_size) {
                        ed->hex_nibble = val;
                    }
                }
            }
            break;
    }

    clamp_cursor(ed);
    ensure_visible(ed, data_rows);
}
