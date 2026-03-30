#include "panel.h"
#include "platform.h"
#include <fnmatch.h>
#include <strings.h>
#include <libgen.h>

static const Panel *g_sort_panel = NULL;

static const char *get_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

static int entry_cmp(const void *a, const void *b) {
    const DirEntry *ea = a, *eb = b;
    const Panel *p = g_sort_panel;
    
    if (!p) return 0;
    
    // Parent (..) always first
    if (ea->type == ENTRY_PARENT) return -1;
    if (eb->type == ENTRY_PARENT) return 1;
    
    // Directories before files (except parent which is already handled)
    int dir_cmp = 0;
    if (ea->type == ENTRY_DIR && eb->type != ENTRY_DIR) dir_cmp = -1;
    else if (ea->type != ENTRY_DIR && eb->type == ENTRY_DIR) dir_cmp = 1;
    
    int cmp = 0;
    switch (p->sort_mode) {
        case SORT_SIZE:
            cmp = (ea->size > eb->size) - (ea->size < eb->size);
            break;
        case SORT_DATE:
            cmp = (ea->mtime > eb->mtime) - (ea->mtime < eb->mtime);
            break;
        case SORT_EXT: {
            const char *exta = get_ext(ea->name);
            const char *extb = get_ext(eb->name);
            cmp = strcasecmp(exta, extb);
            if (cmp == 0) cmp = strcasecmp(ea->name, eb->name);
            break;
        }
        case SORT_NAME:
        default:
            cmp = strcasecmp(ea->name, eb->name);
            break;
    }
    
    // Apply directory sorting first, then the selected sort mode
    if (dir_cmp != 0) {
        return p->sort_reverse ? -dir_cmp : dir_cmp;
    }
    return p->sort_reverse ? -cmp : cmp;
}

int panel_init(Panel *p, const char *path) {
    memset(p, 0, sizeof(Panel));
    p->sort_mode = SORT_NAME;
    p->sort_reverse = false;
    if (path) {
        strncpy(p->path, path, MAX_PATH - 1);
    } else {
        char *cwd = pl_get_cwd();
        if (cwd) strncpy(p->path, cwd, MAX_PATH - 1);
    }
    return panel_refresh(p);
}

void panel_free(Panel *p) {
    free(p->entries);
    p->entries = NULL;
    p->count = 0;
}

int panel_refresh(Panel *p) {
    free(p->entries);
    p->entries = NULL;
    p->count = 0;
    
    PlatformDirEntry *raw_entries;
    int raw_count;
    if (pl_list_dir(p->path, &raw_entries, &raw_count) != 0) {
        return -1;
    }
    
    int has_parent = (strcmp(p->path, "/") != 0);
    p->entries = xmalloc((raw_count + has_parent + 1) * sizeof(DirEntry));
    int out_idx = 0;
    
    if (has_parent) {
        DirEntry *de = &p->entries[out_idx++];
        strcpy(de->name, "..");
        de->type = ENTRY_PARENT;
        de->size = 0;
        de->mtime = 0;
        de->selected = false;
    }
    
    for (int i = 0; i < raw_count; i++) {
        if (!p->show_hidden && raw_entries[i].name[0] == '.') {
            continue;
        }
        if (p->search_filter[0] && fnmatch(p->search_filter, raw_entries[i].name, 0) != 0) {
            continue;
        }
        
        DirEntry *de = &p->entries[out_idx++];
        strncpy(de->name, raw_entries[i].name, MAX_NAME - 1);
        de->path[0] = '\0';
        de->type = raw_entries[i].type;
        de->size = raw_entries[i].size;
        de->mtime = raw_entries[i].mtime;
        de->selected = false;
    }
    p->count = out_idx;
    free(raw_entries);
    
    // Sort the entries (parent is handled by comparator)
    if (p->count > 0) {
        g_sort_panel = p;
        qsort(p->entries, p->count, sizeof(DirEntry), entry_cmp);
        g_sort_panel = NULL;
    }
    
    if (p->cursor >= p->count) p->cursor = p->count - 1;
    if (p->cursor < 0) p->cursor = 0;
    
    return 0;
}

int panel_cursor_up(Panel *p) {
    if (p->cursor > 0) {
        p->cursor--;
        if (p->cursor < p->scroll_offset) {
            p->scroll_offset = p->cursor;
        }
        return 0;
    }
    return -1;
}

int panel_cursor_down(Panel *p) {
    if (p->cursor < p->count - 1) {
        p->cursor++;
        int vis_lines = 15;
        if (p->cursor >= p->scroll_offset + vis_lines) {
            p->scroll_offset = p->cursor - vis_lines / 2;
            if (p->scroll_offset < 0) p->scroll_offset = 0;
        }
        return 0;
    }
    return -1;
}

int panel_enter(Panel *p) {
    if (p->count == 0) return -1;
    DirEntry *e = &p->entries[p->cursor];
    
    if (e->type == ENTRY_PARENT) {
        return panel_parent(p);
    }
    
    if (e->type != ENTRY_DIR) return -1;
    
    char *new_path = pl_path_join(p->path, e->name);
    if (panel_goto(p, new_path) == 0) {
        free(new_path);
        return 0;
    }
    free(new_path);
    return -1;
}

int panel_parent(Panel *p) {
    char *parent = xstrdup(p->path);
    char *last = strrchr(parent, '/');
    if (last && last != parent) {
        *last = '\0';
        if (panel_goto(p, parent) == 0) {
            free(parent);
            return 0;
        }
    }
    free(parent);
    return -1;
}

int panel_goto(Panel *p, const char *path) {
    strncpy(p->path, path, MAX_PATH - 1);
    p->path[MAX_PATH - 1] = '\0';
    p->search_filter[0] = '\0';
    p->cursor = 0;
    p->scroll_offset = 0;
    return panel_refresh(p);
}

void panel_apply_filter(Panel *p) {
    panel_refresh(p);
}

int panel_get_selected_count(Panel *p) {
    int count = 0;
    for (int i = 0; i < p->count; i++) {
        if (p->entries[i].selected) count++;
    }
    return count;
}

uint64_t panel_get_total_size(Panel *p) {
    uint64_t total = 0;
    for (int i = 0; i < p->count; i++) {
        if (p->entries[i].type != ENTRY_DIR && p->entries[i].type != ENTRY_PARENT) {
            total += p->entries[i].size;
        }
    }
    return total;
}

uint64_t panel_get_selected_size(Panel *p) {
    uint64_t total = 0;
    for (int i = 0; i < p->count; i++) {
        if (p->entries[i].selected && p->entries[i].type != ENTRY_DIR && p->entries[i].type != ENTRY_PARENT) {
            total += p->entries[i].size;
        }
    }
    return total;
}

void panel_select_all(Panel *p, bool select) {
    for (int i = 0; i < p->count; i++) {
        p->entries[i].selected = select;
    }
}

void panel_select_pattern(Panel *p, const char *pattern, bool select) {
    for (int i = 0; i < p->count; i++) {
        if (fnmatch(pattern, p->entries[i].name, 0) == 0) {
            p->entries[i].selected = select;
        }
    }
}

void panel_cycle_sort(Panel *p) {
    p->sort_mode = (p->sort_mode + 1) % 4;
    panel_refresh(p);
}

void panel_toggle_sort_reverse(Panel *p) {
    p->sort_reverse = !p->sort_reverse;
    panel_refresh(p);
}

const char* panel_get_sort_name(Panel *p) {
    static const char *names[] = {"name", "size", "date", "ext"};
    return names[p->sort_mode];
}
