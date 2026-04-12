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
    
    // Nord-inspired dark theme palette
    // Background: -1 (Default terminal background)
    
    init_pair(COLOR_BORDER, 14, -1);       // Nord frost blue (borders)
    init_pair(COLOR_SELECTED, 0, 11);      // Black on yellow (cursor)
    init_pair(COLOR_STATUS, 15, 8);        // White on dark gray (status bar)
    init_pair(COLOR_PREVIEW, 12, -1);      // Nord frost cyan (preview)
    
    init_pair(COLOR_PARENT, 13, -1);       // Nord purple
    init_pair(COLOR_DIR, 6, -1);           // Nord frost cyan (directories)
    init_pair(COLOR_SYMLINK, 14, -1);      // Nord frost blue
    init_pair(COLOR_EXEC, 10, -1);         // Nord green (executable)
    init_pair(COLOR_FILE, 7, -1);          // Nord snow storm (white)
    init_pair(COLOR_HIDDEN, 8, -1);        // Dim gray
    init_pair(COLOR_ARCHIVE, 1, -1);       // Nord red (archives)
    init_pair(COLOR_MEDIA, 5, -1);         // Nord magenta (media)
    init_pair(COLOR_CODE, 3, -1);          // Nord yellow (code)
    
    // Additional UI colors
    init_pair(20, 0, 6);                   // Black on cyan (active tab/mode)
    init_pair(21, 15, 4);                  // White on blue (info/path)
    init_pair(22, 15, 2);                  // White on green (success/confirm)
    init_pair(23, 15, 1);                  // White on red (error/warning)
    
    // Syntax highlighting colors
    init_pair(30, 3, -1);                  // Keywords (yellow)
    init_pair(31, 2, -1);                  // Strings (green)
    init_pair(32, 8, -1);                  // Comments (dim gray)
    init_pair(33, 5, -1);                  // Numbers (magenta)
    init_pair(34, 6, -1);                  // Types (cyan)
    init_pair(35, 0, 11);                  // Search match highlight (black on yellow)
    init_pair(36, 5, -1);                  // Preprocessor/special (magenta)
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
    int w = g_layout.width - 20;
    if (w < 40) w = 40;
    if (w > 100) w = 100;
    int h = 7;
    int x = (g_layout.width - w) / 2;
    int y = (g_layout.height - h) / 2;
    
    int percent = (total > 0) ? (int)(done * 100 / total) : 0;
    int bar_w = w - 6;
    int filled = (percent * bar_w) / 100;
    
    attron(COLOR_PAIR(COLOR_STATUS));
    // Draw dialog background
    for (int i = 0; i < h; i++) {
        mvhline(y + i, x, ' ', w);
    }
    
    // Draw dialog border
    attron(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    mvaddstr(y, x, "┏");
    mvwhline(stdscr, y, x + 1, 0x2501, w - 2);
    mvaddstr(y, x + w - 1, "┓");
    mvvline(y + 1, x, 0x2503, h - 2);
    mvvline(y + 1, x + w - 1, 0x2503, h - 2);
    mvaddstr(y + h - 1, x, "┗");
    mvwhline(stdscr, y + h - 1, x + 1, 0x2501, w - 2);
    mvaddstr(y + h - 1, x + w - 1, "┛");
    attroff(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    
    attron(COLOR_PAIR(COLOR_STATUS) | A_BOLD);
    mvprintw(y, x + 2, " %s ", title);
    attroff(A_BOLD);
    
    char fn[MAX_PATH];
    snprintf(fn, sizeof(fn), "%.*s", w - 10, filename);
    mvprintw(y + 2, x + 3, "%s", fn);
    
    // Draw progress bar
    mvprintw(y + 4, x + 3, "[");
    attron(COLOR_PAIR(20));
    for (int i = 0; i < bar_w; i++) {
        if (i < filled) {
            mvaddstr(y + 4, x + 4 + i, "█");
        } else {
            mvaddstr(y + 4, x + 4 + i, "░");
        }
    }
    attroff(COLOR_PAIR(20));
    mvprintw(y + 4, x + 4 + bar_w, "] %3d%%", percent);
    
    attroff(COLOR_PAIR(COLOR_STATUS));
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
    
    ui_draw_preview(&g_state.panels[PANEL_LEFT], x, l.height - l.preview_height - 1, l.panel_width, l.preview_height);
    ui_draw_preview(&g_state.panels[PANEL_RIGHT], x + l.panel_width + 1, l.height - l.preview_height - 1, l.panel_width, l.preview_height);
    
    Panel *active = &g_state.panels[g_state.active];
    int sel_count = panel_get_selected_count(active);
    uint64_t total_size = panel_get_total_size(active);
    uint64_t selected_size = panel_get_selected_size(active);
    
    char size_str[32], sel_str[32];
    if (total_size < 1024 * 1024) {
        snprintf(size_str, sizeof(size_str), "%luK", (unsigned long)(total_size / 1024));
    } else if (total_size < 1024ULL * 1024 * 1024) {
        snprintf(size_str, sizeof(size_str), "%.1fM", (double)total_size / (1024 * 1024));
    } else {
        snprintf(size_str, sizeof(size_str), "%.1fG", (double)total_size / (1024ULL * 1024 * 1024));
    }
    
    if (selected_size > 0) {
        if (selected_size < 1024 * 1024) {
            snprintf(sel_str, sizeof(sel_str), " [%d sel, %luK]", sel_count, (unsigned long)(selected_size / 1024));
        } else if (selected_size < 1024ULL * 1024 * 1024) {
            snprintf(sel_str, sizeof(sel_str), " [%d sel, %.1fM]", sel_count, (double)selected_size / (1024 * 1024));
        } else {
            snprintf(sel_str, sizeof(sel_str), " [%d sel, %.1fG]", sel_count, (double)selected_size / (1024ULL * 1024 * 1024));
        }
    } else {
        sel_str[0] = '\0';
    }
    
    char left_info[256];
    snprintf(left_info, sizeof(left_info), " %d items, %s%s ", active->count, size_str, sel_str);
    
    char sort_info[64];
    snprintf(sort_info, sizeof(sort_info), " %s%s ", panel_get_sort_name(active), active->sort_reverse ? " ↓" : " ↑");
    
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char time_str[16];
    strftime(time_str, sizeof(time_str), " %H:%M ", tm);
    
    // Draw status bar background
    attron(COLOR_PAIR(COLOR_STATUS));
    mvhline(l.height - 1, 0, ' ', l.width);
    attroff(COLOR_PAIR(COLOR_STATUS));
    
    int curr_x = 0;
    
    // Mode Segment
    attron(COLOR_PAIR(20) | A_BOLD);
    const char *mode_name = " NORMAL ";
    if (g_state.mode == MODE_SEARCH) mode_name = " SEARCH ";
    else if (g_state.mode == MODE_INPUT) mode_name = " INPUT  ";
    else if (g_state.mode == MODE_HEXEDIT) mode_name = " HEXEDIT";
    else if (g_state.mode == MODE_TEXTEDIT) mode_name = " TEXTEDI";
    mvprintw(l.height - 1, curr_x, "%s", mode_name);
    curr_x += strlen(mode_name);
    attroff(COLOR_PAIR(20) | A_BOLD);
    
    // Info Segment
    attron(COLOR_PAIR(21));
    mvprintw(l.height - 1, curr_x, "%s", left_info);
    curr_x += strlen(left_info);
    attroff(COLOR_PAIR(21));
    
    // Sort Segment
    attron(COLOR_PAIR(COLOR_STATUS));
    mvprintw(l.height - 1, curr_x, "%s", sort_info);
    curr_x += strlen(sort_info);
    attroff(COLOR_PAIR(COLOR_STATUS));
    
    // Status Message or Hints
    if (g_state.status_msg[0]) {
        attron(COLOR_PAIR(22) | A_BOLD);
        int msg_len = strlen(g_state.status_msg) + 4;
        int msg_x = (l.width - msg_len) / 2;
        if (msg_x < curr_x) msg_x = curr_x + 2;
        mvprintw(l.height - 1, msg_x, "  %s  ", g_state.status_msg);
        attroff(COLOR_PAIR(22) | A_BOLD);
    } else {
        DirEntry *e = &active->entries[active->cursor];
        char hint[128] = "";
        if (active->count > 0) {
            if (e->type == ENTRY_DIR) snprintf(hint, sizeof(hint), "Enter:open  Bksp:up");
            else if (e->type == ENTRY_PARENT) snprintf(hint, sizeof(hint), "Bksp:go up");
            else snprintf(hint, sizeof(hint), "F3:view  F5:copy  F6:move");
        }
        
        int hint_x = l.width - strlen(time_str) - strlen(hint) - 2;
        if (hint_x > curr_x + 5) {
            attron(COLOR_PAIR(COLOR_STATUS) | A_DIM);
            mvprintw(l.height - 1, hint_x, "%s", hint);
            attroff(COLOR_PAIR(COLOR_STATUS) | A_DIM);
        }
    }
    
    // Time Segment
    attron(COLOR_PAIR(20) | A_BOLD);
    mvprintw(l.height - 1, l.width - strlen(time_str), "%s", time_str);
    attroff(COLOR_PAIR(20) | A_BOLD);
    
    refresh();
}

static const char* get_file_icon(const char *name, EntryType type) {
    if (type == ENTRY_PARENT) return "..";
    if (type == ENTRY_DIR) return "📁";
    if (type == ENTRY_SYMLINK) return "🔗";
    
    const char *ext = strrchr(name, '.');
    if (!ext) return "📄";
    ext++;
    
    if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "tar") == 0 ||
        strcasecmp(ext, "gz") == 0 || strcasecmp(ext, "bz2") == 0 ||
        strcasecmp(ext, "xz") == 0 || strcasecmp(ext, "7z") == 0 ||
        strcasecmp(ext, "rar") == 0) return "📦";
    
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0 ||
        strcasecmp(ext, "png") == 0 || strcasecmp(ext, "gif") == 0 ||
        strcasecmp(ext, "bmp") == 0 || strcasecmp(ext, "svg") == 0 ||
        strcasecmp(ext, "webp") == 0 || strcasecmp(ext, "ico") == 0) return "🖼️";
    
    if (strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "wav") == 0 ||
        strcasecmp(ext, "flac") == 0 || strcasecmp(ext, "ogg") == 0 ||
        strcasecmp(ext, "m4a") == 0 || strcasecmp(ext, "aac") == 0) return "🎵";
    
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "avi") == 0 ||
        strcasecmp(ext, "mkv") == 0 || strcasecmp(ext, "mov") == 0 ||
        strcasecmp(ext, "webm") == 0 || strcasecmp(ext, "flv") == 0) return "🎬";
    
    if (strcasecmp(ext, "c") == 0 || strcasecmp(ext, "h") == 0 ||
        strcasecmp(ext, "cpp") == 0 || strcasecmp(ext, "py") == 0 ||
        strcasecmp(ext, "js") == 0 || strcasecmp(ext, "ts") == 0 ||
        strcasecmp(ext, "rs") == 0 || strcasecmp(ext, "go") == 0) return "⚙️";
    
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "css") == 0 ||
        strcasecmp(ext, "json") == 0 || strcasecmp(ext, "xml") == 0) return "🌐";
    
    if (strcasecmp(ext, "md") == 0 || strcasecmp(ext, "txt") == 0 ||
        strcasecmp(ext, "doc") == 0 || strcasecmp(ext, "docx") == 0) return "📝";
    
    if (strcasecmp(ext, "pdf") == 0) return "📕";
    
    if (strcasecmp(ext, "sh") == 0 || strcasecmp(ext, "bash") == 0 ||
        strcasecmp(ext, "zsh") == 0) return "💻";
    
    return "📄";
}

static int get_file_color(const char *name, EntryType type) {
    if (type == ENTRY_PARENT) return COLOR_PARENT;
    if (type == ENTRY_DIR) return COLOR_DIR;
    if (type == ENTRY_SYMLINK) return COLOR_SYMLINK;
    
    const char *ext = strrchr(name, '.');
    if (!ext) return COLOR_FILE;
    ext++;
    
    if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "tar") == 0 ||
        strcasecmp(ext, "gz") == 0 || strcasecmp(ext, "bz2") == 0 ||
        strcasecmp(ext, "xz") == 0 || strcasecmp(ext, "7z") == 0 ||
        strcasecmp(ext, "rar") == 0) {
        return COLOR_ARCHIVE;
    }
    
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0 ||
        strcasecmp(ext, "png") == 0 || strcasecmp(ext, "gif") == 0 ||
        strcasecmp(ext, "mp3") == 0 || strcasecmp(ext, "mp4") == 0 ||
        strcasecmp(ext, "avi") == 0 || strcasecmp(ext, "mkv") == 0) {
        return COLOR_MEDIA;
    }
    
    if (strcasecmp(ext, "c") == 0 || strcasecmp(ext, "h") == 0 ||
        strcasecmp(ext, "cpp") == 0 || strcasecmp(ext, "py") == 0 ||
        strcasecmp(ext, "js") == 0 || strcasecmp(ext, "html") == 0 ||
        strcasecmp(ext, "css") == 0 || strcasecmp(ext, "java") == 0 ||
        strcasecmp(ext, "rs") == 0 || strcasecmp(ext, "go") == 0) {
        return COLOR_CODE;
    }
    
    return COLOR_FILE;
}

static char* shorten_path(const char *path) {
    static char shortened[MAX_PATH];
    const char *home = getenv("HOME");
    if (home && strncmp(path, home, strlen(home)) == 0) {
        snprintf(shortened, sizeof(shortened), "~%s", path + strlen(home));
    } else if (strlen(path) > 40) {
        snprintf(shortened, sizeof(shortened), "...%s", path + strlen(path) - 37);
    } else {
        strncpy(shortened, path, sizeof(shortened) - 1);
        shortened[sizeof(shortened) - 1] = '\0';
    }
    return shortened;
}

void ui_draw_panel(Panel *p, int x, int y, int w, int h, bool active) {
    attron(COLOR_PAIR(COLOR_BORDER));
    if (active) {
        attron(A_BOLD);
    }
    
    // Draw box with UTF-8 border characters
    mvaddstr(y, x, active ? "┏" : "┌");
    mvwhline(stdscr, y, x + 1, active ? 0x2501 : 0x2500, w - 2);
    mvaddstr(y, x + w - 1, active ? "┓" : "┐");
    
    mvvline(y + 1, x, active ? 0x2503 : 0x2502, h - 2);
    mvvline(y + 1, x + w - 1, active ? 0x2503 : 0x2502, h - 2);
    
    mvaddstr(y + h - 1, x, active ? "┗" : "└");
    mvwhline(stdscr, y + h - 1, x + 1, active ? 0x2501 : 0x2500, w - 2);
    mvaddstr(y + h - 1, x + w - 1, active ? "┛" : "┘");
    
    char title[MAX_PATH];
    snprintf(title, sizeof(title), " %s ", shorten_path(p->path));
    int title_len = strlen(title);
    if (title_len > w - 4) {
        title[w - 4] = '\0';
        title_len = w - 4;
    }
    mvprintw(y, x + (w - title_len) / 2, "%s", title);
    
    attroff(COLOR_PAIR(COLOR_BORDER) | A_BOLD);

    int date_w = 12;
    int size_w = 6;
    int icon_w = 4;
    int marker_w = 1;
    int name_w = w - date_w - size_w - icon_w - marker_w - 5;
    if (name_w < 10) name_w = 10;

    for (int i = 0; i < h - 2 && i + p->scroll_offset < p->count; i++) {
        int idx = i + p->scroll_offset;
        DirEntry *e = &p->entries[idx];
        bool is_cursor = (idx == p->cursor);
        int file_color = get_file_color(e->name, e->type);

        if (active && is_cursor) {
            attron(COLOR_PAIR(COLOR_SELECTED));
            // Fill background for the entire line
            mvhline(y + 1 + i, x + 1, ' ', w - 2);
        } else {
            attron(COLOR_PAIR(file_color));
            if (e->type == ENTRY_DIR || e->type == ENTRY_PARENT) {
                attron(A_BOLD);
            }
            mvhline(y + 1 + i, x + 1, ' ', w - 2);
        }

        char datebuf[16];
        char sizebuf[8];
        const char *icon;

        format_date(e->mtime, datebuf, sizeof(datebuf));
        icon = get_file_icon(e->name, e->type);

        if (e->type == ENTRY_PARENT || e->type == ENTRY_DIR) {
            strcpy(sizebuf, "     ");
        } else {
            if (e->size < 1024) {
                snprintf(sizebuf, sizeof(sizebuf), "%5luB", (unsigned long)e->size);
            } else if (e->size < 1024 * 1024) {
                snprintf(sizebuf, sizeof(sizebuf), "%4.1fK", (double)e->size / 1024);
            } else if (e->size < 1024ULL * 1024 * 1024) {
                snprintf(sizebuf, sizeof(sizebuf), "%4.1fM", (double)e->size / (1024 * 1024));
            } else {
                snprintf(sizebuf, sizeof(sizebuf), "%4.1fG", (double)e->size / (1024ULL * 1024 * 1024));
            }
        }

        // Selected marker (*)
        if (e->selected) {
            attron(A_BOLD);
            mvaddstr(y + 1 + i, x + 1, "*");
            attroff(A_BOLD);
        }

        mvprintw(y + 1 + i, x + 3, "%s  %-*.*s %s %s",
                 icon,
                 name_w, name_w, e->name,
                 sizebuf,
                 datebuf);

        if (active && is_cursor) {
            attroff(COLOR_PAIR(COLOR_SELECTED));
        } else {
            attroff(COLOR_PAIR(file_color) | A_BOLD);
        }
    }
}

void ui_draw_preview(Panel *p, int x, int y, int w, int h) {
    if (p->count == 0) return;
    DirEntry *e = &p->entries[p->cursor];
    
    attron(COLOR_PAIR(COLOR_BORDER));
    mvhline(y, x, 0x2500, w);
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
    mvprintw(y++, 6, "F4            Edit file (text or hex)");
    y++;
    mvprintw(y++, 4, "Text Editor (F4 on text files):");
    mvprintw(y++, 6, "Arrows        Move cursor");
    mvprintw(y++, 6, "Home/End      Line start/end");
    mvprintw(y++, 6, "PgUp/PgDn     Page scroll");
    mvprintw(y++, 6, "Insert        Toggle insert/overwrite");
    mvprintw(y++, 6, "F2 / Ctrl+S   Save file");
    mvprintw(y++, 6, "Ctrl+Z / Y    Undo / Redo");
    mvprintw(y++, 6, "Ctrl+A        Select all");
    mvprintw(y++, 6, "Ctrl+C/X/V    Copy / Cut / Paste");
    mvprintw(y++, 6, "/             Find text");
    mvprintw(y++, 6, "n / N         Next / Previous match");
    mvprintw(y++, 6, "Ctrl+H        Find and replace");
    mvprintw(y++, 6, "Ctrl+G        Go to line");
    mvprintw(y++, 6, "Ctrl+K        Delete line");
    mvprintw(y++, 6, "Ctrl+D        Duplicate line");
    mvprintw(y++, 6, "Ctrl+B        Toggle bookmark");
    mvprintw(y++, 6, "Ctrl+N / P    Next / Previous bookmark");
    mvprintw(y++, 6, "Tab / S-Tab   Indent / Dedent");
    mvprintw(y++, 6, "Ctrl+W        Toggle word wrap");
    mvprintw(y++, 6, "Esc           Exit editor");
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
    int msg_len = strlen(msg);
    int w = (msg_len > (int)strlen(title) ? msg_len : (int)strlen(title)) + 10;
    if (w < 40) w = 40;
    if (w > g_layout.width - 4) w = g_layout.width - 4;
    int h = 7;
    int x = (g_layout.width - w) / 2;
    int y = (g_layout.height - h) / 2;
    
    attron(COLOR_PAIR(COLOR_STATUS));
    for (int i = 0; i < h; i++) mvhline(y + i, x, ' ', w);
    
    attron(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    mvaddstr(y, x, "┏");
    mvwhline(stdscr, y, x + 1, 0x2501, w - 2);
    mvaddstr(y, x + w - 1, "┓");
    mvvline(y + 1, x, 0x2503, h - 2);
    mvvline(y + 1, x + w - 1, 0x2503, h - 2);
    mvaddstr(y + h - 1, x, "┗");
    mvwhline(stdscr, y + h - 1, x + 1, 0x2501, w - 2);
    mvaddstr(y + h - 1, x + w - 1, "┛");
    
    mvprintw(y, x + 2, " %s ", title);
    attroff(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    
    attron(COLOR_PAIR(COLOR_STATUS));
    mvprintw(y + 2, x + (w - msg_len) / 2, "%s", msg);
    attron(A_DIM);
    mvprintw(y + 4, x + (w - 20) / 2, "Press any key...");
    attroff(A_DIM | COLOR_PAIR(COLOR_STATUS));
    
    refresh();
    nodelay(stdscr, FALSE);
    getch();
    timeout(100);
}

char *ui_input(const char *prompt, const char *initial) {
    int w = g_layout.width - 20;
    if (w < 40) w = 40;
    if (w > 80) w = 80;
    int h = 5;
    int x = (g_layout.width - w) / 2;
    int y = (g_layout.height - h) / 2;
    
    attron(COLOR_PAIR(COLOR_STATUS));
    for (int i = 0; i < h; i++) mvhline(y + i, x, ' ', w);
    
    attron(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    mvaddstr(y, x, "┏");
    mvwhline(stdscr, y, x + 1, 0x2501, w - 2);
    mvaddstr(y, x + w - 1, "┓");
    mvvline(y + 1, x, 0x2503, h - 2);
    mvvline(y + 1, x + w - 1, 0x2503, h - 2);
    mvaddstr(y + h - 1, x, "┗");
    mvwhline(stdscr, y + h - 1, x + 1, 0x2501, w - 2);
    mvaddstr(y + h - 1, x + w - 1, "┛");
    
    mvprintw(y, x + 2, " %s ", prompt);
    attroff(COLOR_PAIR(COLOR_BORDER) | A_BOLD);
    
    echo();
    curs_set(1);
    
    char buf[MAX_PATH];
    if (initial) strncpy(buf, initial, MAX_PATH - 1);
    else buf[0] = '\0';
    
    attron(COLOR_PAIR(COLOR_STATUS));
    mvprintw(y + 2, x + 2, "> %s", buf);
    int len = strlen(buf);
    move(y + 2, x + 4 + len);
    
    int ch;
    while ((ch = getch()) != ERR) {
        if (ch == '\n' || ch == KEY_ENTER) {
            noecho();
            curs_set(0);
            attroff(COLOR_PAIR(COLOR_STATUS));
            return xstrdup(buf);
        }
        if (ch == 27) {
            noecho();
            curs_set(0);
            attroff(COLOR_PAIR(COLOR_STATUS));
            return NULL;
        }
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (len > 0) {
                buf[--len] = '\0';
                mvhline(y + 2, x + 4, ' ', w - 6);
                mvprintw(y + 2, x + 4, "%s", buf);
                move(y + 2, x + 4 + len);
            }
        }
        else if (ch >= 32 && ch < 127 && len < MAX_PATH - 1) {
            buf[len++] = ch;
            buf[len] = '\0';
            mvaddch(y + 2, x + 3 + len, ch);
            move(y + 2, x + 4 + len);
        }
    }
    
    noecho();
    curs_set(0);
    attroff(COLOR_PAIR(COLOR_STATUS));
    return NULL;
}

int ui_confirm(const char *msg) {
    int msg_len = strlen(msg);
    int w = msg_len + 10;
    if (w < 40) w = 40;
    int h = 5;
    int x = (g_layout.width - w) / 2;
    int y = (g_layout.height - h) / 2;
    
    attron(COLOR_PAIR(COLOR_STATUS));
    for (int i = 0; i < h; i++) mvhline(y + i, x, ' ', w);
    
    attron(COLOR_PAIR(23) | A_BOLD); // Error/Warning color for confirmation
    mvaddstr(y, x, "┏");
    mvwhline(stdscr, y, x + 1, 0x2501, w - 2);
    mvaddstr(y, x + w - 1, "┓");
    mvvline(y + 1, x, 0x2503, h - 2);
    mvvline(y + 1, x + w - 1, 0x2503, h - 2);
    mvaddstr(y + h - 1, x, "┗");
    mvwhline(stdscr, y + h - 1, x + 1, 0x2501, w - 2);
    mvaddstr(y + h - 1, x + w - 1, "┛");
    
    mvprintw(y, x + 2, " Confirm ");
    attroff(COLOR_PAIR(23) | A_BOLD);
    
    attron(COLOR_PAIR(COLOR_STATUS));
    mvprintw(y + 2, x + (w - msg_len) / 2, "%s", msg);
    attron(A_BOLD);
    mvprintw(y + 3, x + (w - 12) / 2, " [Y]es  [N]o ");
    attroff(A_BOLD | COLOR_PAIR(COLOR_STATUS));
    refresh();
    
    int ch;
    while ((ch = getch()) != ERR) {
        if (ch == 'y' || ch == 'Y') return 1;
        if (ch == 'n' || ch == 'N' || ch == 27) return 0;
    }
    return 0;
}