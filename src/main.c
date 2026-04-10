#include "fml.h"
#include "ui.h"
#include "panel.h"
#include "fileops.h"
#include "search.h"
#include "platform.h"
#include "shell.h"
#include "clipboard.h"
#include <locale.h>

AppState g_state;

void die(const char *msg) {
    ui_cleanup();
    fprintf(stderr, "fml: %s\n", msg);
    exit(1);
}

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) die("out of memory");
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    ptr = realloc(ptr, size);
    if (!ptr) die("out of memory");
    return ptr;
}

char *xstrdup(const char *s) {
    char *p = strdup(s);
    if (!p) die("out of memory");
    return p;
}

static void init_state(void) {
    memset(&g_state, 0, sizeof(AppState));
    g_state.running = true;
    g_state.mode = MODE_NORMAL;
    g_state.active = PANEL_LEFT;
}

static void handle_key(int key) {
    if (g_state.mode == MODE_SEARCH) {
        if (key == 27 || key == KEY_CANCEL) {
            search_cancel(&g_state.panels[g_state.active]);
        }
        else if (key == '\n' || key == KEY_ENTER) {
            search_finish(&g_state.panels[g_state.active]);
        }
        else if (key >= 32 && key < 127) {
            search_update(&g_state.panels[g_state.active], (char)key);
        }
        return;
    }
    
    Panel *active = &g_state.panels[g_state.active];
    Panel *other = &g_state.panels[1 - g_state.active];
    
    switch (key) {
        case KEY_UP: case 'k':
            panel_cursor_up(active);
            break;
        case KEY_DOWN: case 'j':
            panel_cursor_down(active);
            break;
        case KEY_LEFT: case 'h':
            panel_parent(active);
            break;
        case KEY_RIGHT: case 'l':
            panel_enter(active);
            break;
        case KEY_ENTER: case '\n':
            panel_enter(active);
            break;
        case KEY_BACKSPACE: case 127:
            panel_parent(active);
            break;
        case KEY_BTAB: case '\t':
            g_state.active = 1 - g_state.active;
            break;
        case KEY_HOME:
            active->cursor = 0;
            active->scroll_offset = 0;
            break;
        case KEY_END: {
            UILayout l = ui_get_layout();
            int vis = l.height - l.preview_height - 4;
            active->cursor = active->count - 1;
            panel_ensure_visible(active, vis);
            break;
        }
        case KEY_PPAGE: {
            UILayout l = ui_get_layout();
            int vis = l.height - l.preview_height - 4;
            active->cursor -= vis;
            if (active->cursor < 0) active->cursor = 0;
            panel_ensure_visible(active, vis);
            break;
        }
        case KEY_NPAGE: {
            UILayout l = ui_get_layout();
            int vis = l.height - l.preview_height - 4;
            active->cursor += vis;
            if (active->cursor >= active->count) active->cursor = active->count - 1;
            panel_ensure_visible(active, vis);
            break;
        }
        case '*': case KEY_IC:
            if (active->count > 0) {
                active->entries[active->cursor].selected = !active->entries[active->cursor].selected;
                panel_cursor_down(active);
            }
            break;
        case '+': {
            char *pattern = ui_input("Select pattern:", "*");
            if (pattern) {
                panel_select_pattern(active, pattern, true);
                free(pattern);
            }
            break;
        }
        case '\\': {
            char *pattern = ui_input("Deselect pattern:", "*");
            if (pattern) {
                panel_select_pattern(active, pattern, false);
                free(pattern);
            }
            break;
        }
        case '/':
            search_start(active);
            break;
        case 's': {
            panel_cycle_sort(active);
            char msg[128];
            snprintf(msg, sizeof(msg), "Sort: %s %s", panel_get_sort_name(active), active->sort_reverse ? "(rev)" : "");
            ui_draw_status(msg);
            break;
        }
        case 'S': {
            panel_toggle_sort_reverse(active);
            char msg[128];
            snprintf(msg, sizeof(msg), "Sort: %s %s", panel_get_sort_name(active), active->sort_reverse ? "(rev)" : "");
            ui_draw_status(msg);
            break;
        }
        case '.':
            active->show_hidden = !active->show_hidden;
            panel_refresh(active);
            ui_draw_status(active->show_hidden ? "Showing hidden files" : "Hiding hidden files");
            break;
        case '!':
            shell_spawn(active->path);
            panel_refresh(active);
            panel_refresh(other);
            break;
        case 19:  // Ctrl+S
            shell_spawn(active->path);
            panel_refresh(active);
            panel_refresh(other);
            break;
        case 3: {  // Ctrl+C - copy filename
            if (active->count > 0) {
                DirEntry *e = &active->entries[active->cursor];
                clipboard_copy_text(e->name);
                ui_draw_status("Filename copied to clipboard");
            }
            break;
        }
        case 25: {  // Ctrl+Y - copy full path
            if (active->count > 0) {
                DirEntry *e = &active->entries[active->cursor];
                char *path = pl_path_join(active->path, e->name);
                clipboard_copy_text(path);
                ui_draw_status("Full path copied to clipboard");
                free(path);
            }
            break;
        }
        case KEY_F(1):
            ui_draw_help();
            break;
        case KEY_F(3):
            g_state.mode = (g_state.mode == MODE_PREVIEW_FULLSCREEN) ? MODE_NORMAL : MODE_PREVIEW_FULLSCREEN;
            break;
        case KEY_F(5): {
            int selected = panel_get_selected_count(active);
            if (selected > 0) {
                char msg[64];
                snprintf(msg, sizeof(msg), "Copy %d selected items?", selected);
                if (ui_confirm(msg)) {
                    fileops_copy(active, other);
                }
            } else if (active->count > 0) {
                fileops_copy(active, other);
            }
            break;
        }
        case KEY_F(6): {
            int selected = panel_get_selected_count(active);
            if (selected > 0) {
                char msg[64];
                snprintf(msg, sizeof(msg), "Move %d selected items?", selected);
                if (ui_confirm(msg)) {
                    fileops_move(active, other, NULL);
                }
            } else if (active->count > 0) {
                char *newname = ui_input("Move/Rename to:", active->entries[active->cursor].name);
                if (newname) {
                    fileops_move(active, other, newname);
                    free(newname);
                }
            }
            break;
        }
        case KEY_F(7): {
            char *name = ui_input("Create directory:", "newdir");
            if (name) {
                fileops_mkdir(active, name);
                free(name);
            }
            break;
        }
        case KEY_F(8): case KEY_DC:
            fileops_delete(active);
            break;
        case KEY_F(9):
        case KEY_F(10):
        case 'q':
            g_state.running = false;
            break;
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    pl_init();
    init_state();
    setlocale(LC_ALL, "");
    
    if (ui_init() != 0) {
        die("failed to initialize UI");
    }
    
    char *cwd = pl_get_cwd();
    panel_init(&g_state.panels[PANEL_LEFT], cwd);
    panel_init(&g_state.panels[PANEL_RIGHT], cwd);
    
    while (g_state.running) {
        ui_draw();
        int key = ui_get_key();
        if (key != -1) {
            handle_key(key);
        }
    }
    
    panel_free(&g_state.panels[PANEL_LEFT]);
    panel_free(&g_state.panels[PANEL_RIGHT]);
    ui_cleanup();
    pl_cleanup();
    
    return 0;
}