#include "ui.h"
#include "panel.h"
#include "preview.h"
#include "platform.h"
#include <ncurses.h>
#include <signal.h>
#include <ctype.h>
#include <strings.h>

static UILayout g_layout;

static void handle_sigwinch(int sig) {
    (void)sig;
    endwin();
    refresh();
    clear();
    ui_resize();
}

static void init_colors(void) {
    if (!has_colors()) return;
    start_color();
    use_default_colors();
    
    // UI Elements
    init_pair(COLOR_BORDER, COLOR_CYAN, -1);
    init_pair(COLOR_SELECTED, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_STATUS, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_PREVIEW, COLOR_GREEN, -1);
    
    // File types with background highlighting
    init_pair(COLOR_PARENT, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_DIR, COLOR_BLUE, -1);
    init_pair(COLOR_SYMLINK, COLOR_CYAN, -1);
    init_pair(COLOR_EXEC, COLOR_GREEN, -1);
    init_pair(COLOR_FILE, COLOR_WHITE, -1);
    init_pair(COLOR_HIDDEN, COLOR_BLACK, -1);
    init_pair(COLOR_ARCHIVE, COLOR_RED, -1);
    init_pair(COLOR_MEDIA, COLOR_MAGENTA, -1);
    init_pair(COLOR_CODE, COLOR_YELLOW, -1);
}

static void format_date(time_t t, char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&t);
    
    if (t == 0) {
        strncpy(buf, "          ", len);
        buf[len-1] = '\0';
        return;
    }
    
    // Within 6 months: show date and time
    // Older: show date and year
    if (now - t < 180 * 24 * 3600) {
        strftime(buf, len, "%b %d %H:%M", tm);
    } else {
        strftime(buf, len, "%b %d  %Y", tm);
    }
}

static int g_cancel_requested = 0;

int ui_progress(const char *title, const char *filename, uint64_t done, uint64_t total) {
    int w = g_layout.width - 10;
    if (w < 40) w = 40;
    int h = 5;
    int x = (g_layout.width - w) / 2;
    int y = g_layout.height / 2 - 2;
    
    int percent = (total > 0) ? (int)(done * 100 / total) : 0;
    int filled = (percent * (w - 6)) / 100;
    if (filled > w - 6) filled = w - 6;
    
    attron(COLOR_PAIR(COLOR_STATUS) | A_BOLD);
    mvwhline(stdscr, y, x, ' ', w);
    mvwhline(stdscr, y + h - 1, x, ' ', w);
    for (int i = 1; i < h - 1; i++) {
        mvwaddch(stdscr, y + i, x, ' ');
        mvwaddch(stdscr, y + i, x + w - 1, ' ');
    }
    mvwaddch(stdscr, y, x, ACS_ULCORNER);
    mvwaddch(stdscr, y, x + w - 1, ACS_URCORNER);
    mvwaddch(stdscr, y + h - 1, x, ACS_LLCORNER);
    mvwaddch(stdscr, y + h - 1, x + w - 1, ACS_LRCORNER);
    whline(stdscr, ACS_HLINE, w - 2);
    mvwaddch(stdscr, y, x + w - 1, ACS_URCORNER);
    
    mvwprintw(stdscr, y, x + 2, "%s", title);
    
    char bar[256];
    memset(bar, '=', filled);
    memset(bar + filled, '-', w - 6 - filled);
    bar[w - 6] = '\0';
    mvwprintw(stdscr, y + 2, x + 2, "[%s] %3d%%", bar, percent);
    
    char fn[MAX_PATH];
    snprintf(fn, sizeof(fn), "%.*s", w - 10, filename);
    mvwprintw(stdscr, y + 3, x + 2, "%s", fn);
    
    attroff(COLOR_PAIR(COLOR_STATUS) | A_BOLD);
    wrefresh(stdscr);
    
    nodelay(stdscr, TRUE);
    int ch = getch();
    nodelay(stdscr, FALSE);
    
    if (ch == 27) {
        return 0;
    }
    return 1;
}

int ui_check_cancel(void) {
    nodelay(stdscr, TRUE);
    int ch = getch();
    nodelay(stdscr, FALSE);
    return (ch == 27) ? -1 : 0;
}

void ui_set_cancel(int cancel) {
    g_cancel_requested = cancel;
}

int ui_init(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);
    
    init_colors();
    
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigwinch;
    sigaction(SIGWINCH, &sa, NULL);
    
    ui_resize();
    return 0;
}

void ui_cleanup(void) {
    endwin();
}

void ui_resize(void) {
    getmaxyx(stdscr, g_layout.height, g_layout.width);
    if (g_layout.width < UI_MIN_WIDTH) g_layout.width = UI_MIN_WIDTH;
    if (g_layout.height < UI_MIN_HEIGHT) g_layout.height = UI_MIN_HEIGHT;
    
    g_layout.panel_width = (g_layout.width - 3) / 2;
    g_layout.preview_height = g_layout.height / 3;
}

UILayout ui_get_layout(void) {
    return g_layout;
}

int ui_get_key(void) {
    int ch = getch();
    if (ch == ERR) return -1;
    return ch;
}

void ui_beep(void) {
    beep();
}

void ui_draw(void) {
    erase();
    UILayout l = g_layout;
    int x = 0;
    
    ui_draw_panel(&g_state.panels[PANEL_LEFT], x, 0, l.panel_width, l.height - l.preview_height - 2, g_state.active == PANEL_LEFT);
    ui_draw_panel(&g_state.panels[PANEL_RIGHT], x + l.panel_width + 1, 0, l.panel_width, l.height - l.preview_height - 2, g_state.active == PANEL_RIGHT);
    
    ui_draw_preview(&g_state.panels[g_state.active], x, l.height - l.preview_height - 1, l.panel_width, l.preview_height);
    ui_draw_preview(&g_state.panels[1 - g_state.active], x + l.panel_width + 1, l.height - l.preview_height - 1, l.panel_width, l.preview_height);
    
    // Draw status bar
    Panel *active = &g_state.panels[g_state.active];
    char left_status[128];
    int sel_count = panel_get_selected_count(active);
    uint64_t total_size = panel_get_total_size(active);
    
    if (total_size < 1024 * 1024) {
        snprintf(left_status, sizeof(left_status), "%d files, %d sel, %luK | Sort:%s%s", 
                 active->count, sel_count, (unsigned long)(total_size / 1024),
                 panel_get_sort_name(active), active->sort_reverse ? "r" : "");
    } else if (total_size < 1024ULL * 1024 * 1024) {
        snprintf(left_status, sizeof(left_status), "%d files, %d sel, %luM | Sort:%s%s", 
                 active->count, sel_count, (unsigned long)(total_size / (1024 * 1024)),
                 panel_get_sort_name(active), active->sort_reverse ? "r" : "");
    } else {
        snprintf(left_status, sizeof(left_status), "%d files, %d sel, %luG | Sort:%s%s", 
                 active->count, sel_count, (unsigned long)(total_size / (1024ULL * 1024 * 1024)),
                 panel_get_sort_name(active), active->sort_reverse ? "r" : "");
    }
    
    attron(COLOR_PAIR(COLOR_STATUS));
    mvhline(l.height - 1, 0, ' ', l.width);
    mvprintw(l.height - 1, 0, " %s", left_status);
    
    const char *keys = "F1Help F3View F5Copy F6Move F7MkDir F8Del F9Quit";
    mvprintw(l.height - 1, l.width - strlen(keys) - 1, "%s ", keys);
    
    if (g_state.status_msg[0]) {
        int msg_x = (l.width - strlen(g_state.status_msg)) / 2;
        mvprintw(l.height - 1, msg_x, " %s ", g_state.status_msg);
    }
    attroff(COLOR_PAIR(COLOR_STATUS));
    
    refresh();
}

static int get_file_color(const char *name, EntryType type) {
    if (type == ENTRY_PARENT) return COLOR_PARENT;
    if (type == ENTRY_DIR) return COLOR_DIR;
    if (type == ENTRY_SYMLINK) return COLOR_SYMLINK;
    
    const char *ext = strrchr(name, '.');
    if (!ext) return COLOR_FILE;
    ext++;
    
    // Archives
    if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "tar") == 0 ||
        strcasecmp(ext, "gz") == 0 || strcasecmp(ext, "bz2") == 0 ||
        strcasecmp(ext, "xz") == 0 || strcasecmp(ext, "7z") == 0 ||
        strcasecmp(ext, "rar") == 0) {
        return COLOR_ARCHIVE;
    }
    
    // Media
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0 ||
        strcasecmp(ext, "png") == 0 || strcasecmp(ext, "gif") == 0 ||
        strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "mp4") == 0 ||
        strcasecmp(ext, "avi") == 0 || strcasecmp(ext, "mkv") == 0) {
        return COLOR_MEDIA;
    }
    
    // Code
    if (strcasecmp(ext, "c") == 0 || strcasecmp(ext, "h") == 0 ||
        strcasecmp(ext, "cpp") == 0 || strcasecmp(ext, "py") == 0 ||
        strcasecmp(ext, "js") == 0 || strcasecmp(ext, "html") == 0 ||
        strcasecmp(ext, "css") == 0 || strcasecmp(ext, "java") == 0 ||
        strcasecmp(ext, "rs") == 0 || strcasecmp(ext, "go") == 0) {
        return COLOR_CODE;
    }
    
    return COLOR_FILE;
}

void ui_draw_panel(Panel *p, int x, int y, int w, int h, bool active) {
    attron(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    mvhline(y, x, ACS_HLINE, w);
    mvhline(y + h - 1, x, ACS_HLINE, w);
    mvvline(y, x, ACS_VLINE, h);
    mvvline(y, x + w - 1, ACS_VLINE, h);
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(COLOR_BORDER) | A_BOLD);

    if (active) {
        attron(A_BOLD | COLOR_PAIR(COLOR_BORDER));
    } else {
        attron(COLOR_PAIR(COLOR_BORDER));
    }
    char title[MAX_PATH];
    snprintf(title, sizeof(title), " %.40s ", p->path);
    mvprintw(y, x + (w - (int)strlen(title)) / 2, "%s", title);
    if (active) {
        attroff(A_BOLD | COLOR_PAIR(COLOR_BORDER));
    } else {
        attroff(COLOR_PAIR(COLOR_BORDER));
    }

    int date_w = 12;
    int size_w = 5;
    int type_w = 4;
    int marker_w = 2;
    int name_w = w - date_w - size_w - type_w - marker_w - 6;
    if (name_w < 10) name_w = 10;

    for (int i = 0; i < h - 2 && i + p->scroll_offset < p->count; i++) {
        int idx = i + p->scroll_offset;
        DirEntry *e = &p->entries[idx];
        bool is_cursor = (idx == p->cursor);
        int file_color = get_file_color(e->name, e->type);

        // Clear the line first
        mvhline(y + 1 + i, x + 1, ' ', w - 2);

        if (active && is_cursor) {
            // Selected item gets full background highlight
            attron(COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
        } else {
            // Non-selected items get subtle color coding
            if (e->type == ENTRY_PARENT) {
                attron(COLOR_PAIR(COLOR_PARENT) | A_BOLD);
            } else if (e->type == ENTRY_DIR) {
                attron(COLOR_PAIR(COLOR_DIR) | A_BOLD);
            } else if (e->type == ENTRY_SYMLINK) {
                attron(COLOR_PAIR(COLOR_SYMLINK));
            } else {
                attron(COLOR_PAIR(file_color));
            }
        }

        char datebuf[16];
        char sizebuf[8];
        const char *typestr;

        format_date(e->mtime, datebuf, sizeof(datebuf));

        if (e->type == ENTRY_PARENT) {
            typestr = "<UP>";
            strcpy(sizebuf, "    ");
        } else if (e->type == ENTRY_DIR) {
            typestr = "DIR ";
            strcpy(sizebuf, "    ");
        } else if (e->type == ENTRY_SYMLINK) {
            typestr = "LNK ";
            strcpy(sizebuf, "    ");
        } else {
            typestr = "    ";
            if (e->size < 1024) {
                snprintf(sizebuf, sizeof(sizebuf), "%3luB", (unsigned long)e->size);
            } else if (e->size < 1024 * 1024) {
                snprintf(sizebuf, sizeof(sizebuf), "%3luK", (unsigned long)(e->size / 1024));
            } else if (e->size < 1024ULL * 1024 * 1024) {
                snprintf(sizebuf, sizeof(sizebuf), "%3luM", (unsigned long)(e->size / (1024 * 1024)));
            } else {
                snprintf(sizebuf, sizeof(sizebuf), "%3luG", (unsigned long)(e->size / (1024ULL * 1024 * 1024)));
            }
        }

        mvprintw(y + 1 + i, x + 1, "%c%c%s %-*.*s %*s %s",
                 is_cursor ? '>' : ' ',
                 e->selected ? '*' : ' ',
                 typestr,
                 name_w, name_w, e->name,
                 size_w, sizebuf,
                 datebuf);

        if (active && is_cursor) {
            attroff(COLOR_PAIR(COLOR_SELECTED) | A_BOLD);
        } else {
            if (e->type == ENTRY_PARENT) {
                attroff(COLOR_PAIR(COLOR_PARENT) | A_BOLD);
            } else if (e->type == ENTRY_DIR) {
                attroff(COLOR_PAIR(COLOR_DIR) | A_BOLD);
            } else if (e->type == ENTRY_SYMLINK) {
                attroff(COLOR_PAIR(COLOR_SYMLINK));
            } else {
                attroff(COLOR_PAIR(file_color));
            }
        }
    }
}

void ui_draw_preview(Panel *p, int x, int y, int w, int h) {
    if (p->count == 0) return;
    DirEntry *e = &p->entries[p->cursor];
    
    attron(COLOR_PAIR(COLOR_BORDER));
    mvhline(y, x, ACS_HLINE, w);
    attroff(COLOR_PAIR(COLOR_BORDER));
    
    char *path = pl_path_join(p->path, e->name);
    char *content = preview_get(path, w - 2, h - 1);
    free(path);
    
    if (content) {
        attron(COLOR_PAIR(COLOR_PREVIEW));
        int line = y + 1;
        char *saveptr;
        char *line_str = strtok_r(content, "\n", &saveptr);
        while (line_str && line < y + h) {
            mvprintw(line, x + 1, "%-*.*s", w - 2, w - 2, line_str);
            line_str = strtok_r(NULL, "\n", &saveptr);
            line++;
        }
        attroff(COLOR_PAIR(COLOR_PREVIEW));
        free(content);
    }
}

void ui_draw_status(const char *msg) {
    strncpy(g_state.status_msg, msg, MAX_LINE - 1);
    g_state.status_msg[MAX_LINE - 1] = '\0';
}

void ui_draw_help(void) {
    erase();
    int y = 2;
    mvprintw(y++, 2, "fml - Dual Pane File Manager v%s", VERSION);
    y++;
    mvprintw(y++, 4, "Navigation:");
    mvprintw(y++, 6, "Up/Down/k/j   Move cursor");
    mvprintw(y++, 6, "Enter/l       Enter directory / open file");
    mvprintw(y++, 6, "Backspace/h   Go to parent directory");
    mvprintw(y++, 6, "Tab           Switch between panels");
    y++;
    mvprintw(y++, 4, "Sorting & View:");
    mvprintw(y++, 6, "s             Cycle sort mode (name/size/date/ext)");
    mvprintw(y++, 6, "S             Reverse sort order");
    mvprintw(y++, 6, ".             Toggle hidden files");
    mvprintw(y++, 6, "F3            Toggle full-screen preview");
    y++;
    mvprintw(y++, 4, "File Operations:");
    mvprintw(y++, 6, "F5            Copy/Extract archive to other panel");
    mvprintw(y++, 6, "F6            Move/Rename selected");
    mvprintw(y++, 6, "F8/Del        Delete selected");
    mvprintw(y++, 6, "F7            Create directory");
    y++;
    mvprintw(y++, 4, "Selection:");
    mvprintw(y++, 6, "Insert/*      Toggle selection on current item");
    mvprintw(y++, 6, "+             Select files matching pattern");
    mvprintw(y++, 6, "\\             Deselect files matching pattern");
    y++;
    mvprintw(y++, 4, "Shell & Clipboard:");
    mvprintw(y++, 6, "! / Ctrl+S    Spawn shell in current directory");
    mvprintw(y++, 6, "Ctrl+C        Copy filename to clipboard");
    mvprintw(y++, 6, "Ctrl+Y        Copy full path to clipboard");
    y++;
    mvprintw(y++, 4, "Other:");
    mvprintw(y++, 6, "/             Quick search/filter");
    mvprintw(y++, 6, "F1            Show this help");
    mvprintw(y++, 6, "F9/F10/q      Quit");
    y++;
    mvprintw(y++, 2, "Press any key to close");
    refresh();
    nodelay(stdscr, FALSE);
    getch();
    timeout(100);
}

void ui_message(const char *title, const char *msg) {
    int w = strlen(title) + strlen(msg) + 8;
    if (w < 40) w = 40;
    int h = 5;
    int x = (g_layout.width - w) / 2;
    int y = (g_layout.height - h) / 2;
    
    attron(COLOR_PAIR(COLOR_STATUS));
    mvhline(y, x, ' ', w);
    mvprintw(y, x + (w - strlen(title)) / 2, "%s", title);
    mvhline(y + 1, x + 1, ' ', w - 2);
    mvprintw(y + 2, x + (w - strlen(msg)) / 2, "%s", msg);
    mvhline(y + 3, x + 1, ' ', w - 2);
    mvhline(y + 4, x, ' ', w);
    attroff(COLOR_PAIR(COLOR_STATUS));
    refresh();
    nodelay(stdscr, FALSE);
    getch();
    timeout(100);
}

char *ui_input(const char *prompt, const char *initial) {
    int w = g_layout.width - 10;
    int h = 3;
    int x = 5;
    int y = g_layout.height / 2;
    
    attron(COLOR_PAIR(COLOR_STATUS));
    mvhline(y, x, ' ', w);
    mvprintw(y, x, "%s", prompt);
    mvhline(y + 1, x, ' ', w);
    mvhline(y + 2, x, ' ', w);
    attroff(COLOR_PAIR(COLOR_STATUS));
    
    echo();
    curs_set(1);
    
    char buf[MAX_PATH];
    if (initial) strncpy(buf, initial, MAX_PATH - 1);
    else buf[0] = '\0';
    
    mvprintw(y + 1, x + 1, "%s", buf);
    int len = strlen(buf);
    move(y + 1, x + 1 + len);
    
    int ch;
    while ((ch = getch()) != ERR) {
        if (ch == '\n' || ch == KEY_ENTER) {
            noecho();
            curs_set(0);
            return xstrdup(buf);
        }
        if (ch == 27 || ch == KEY_CANCEL) {
            noecho();
            curs_set(0);
            return NULL;
        }
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (len > 0) {
                buf[--len] = '\0';
                mvprintw(y + 1, x + 1, "%s ", buf);
                move(y + 1, x + 1 + len);
            }
        }
        else if (ch >= 32 && ch < 127 && len < MAX_PATH - 1) {
            buf[len++] = ch;
            buf[len] = '\0';
            mvaddch(y + 1, x + len, ch);
            move(y + 1, x + 1 + len);
        }
    }
    
    noecho();
    curs_set(0);
    return NULL;
}

int ui_confirm(const char *msg) {
    int w = strlen(msg) + 20;
    int h = 3;
    int x = (g_layout.width - w) / 2;
    int y = (g_layout.height - h) / 2;
    
    attron(COLOR_PAIR(COLOR_STATUS));
    mvhline(y, x, ' ', w);
    mvprintw(y, x + 2, "%s", msg);
    mvhline(y + 1, x, ' ', w);
    mvprintw(y + 2, x + (w - 12) / 2, "[Y]es [N]o");
    attroff(COLOR_PAIR(COLOR_STATUS));
    refresh();
    
    int ch;
    while ((ch = getch()) != ERR) {
        if (ch == 'y' || ch == 'Y') return 1;
        if (ch == 'n' || ch == 'N' || ch == 27) return 0;
    }
    return 0;
}