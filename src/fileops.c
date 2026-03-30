#include "fileops.h"
#include "panel.h"
#include "platform.h"
#include "ui.h"
#include "archive.h"

static int copy_progress_cb(uint64_t done, uint64_t total, const char *fn, void *ud) {
    (void)ud;
    return ui_progress("Copying...", fn, done, total);
}

int fileops_copy(Panel *src, Panel *dst) {
    int selected = panel_get_selected_count(src);
    if (selected == 0) {
        if (src->count == 0) return -1;
        selected = 1;
    }
    
    for (int i = 0; i < src->count; i++) {
        DirEntry *e = &src->entries[i];
        if (!e->selected && i != src->cursor) continue;
        
        char *src_path = pl_path_join(src->path, e->name);
        char *dst_path = pl_path_join(dst->path, e->name);
        
        int ret;
        
        if (e->type == ENTRY_DIR) {
            ui_draw_status("Copying directory...");
            ret = pl_copy_dir_progress(src_path, dst_path, copy_progress_cb, NULL);
        } else if (archive_is_supported(e->name)) {
            ui_draw_status("Extracting archive...");
            ret = archive_extract(src_path, dst->path);
            free(dst_path);
            dst_path = pl_path_join(dst->path, e->name);
            if (ret == 0) {
                size_t len = strlen(dst_path);
                if (len > 4 && strcasecmp(dst_path + len - 4, ".zip") == 0) {
                    dst_path[len-4] = '\0';
                }
            }
        } else {
            ui_draw_status("Copying file...");
            ret = pl_copy_file_progress(src_path, dst_path, copy_progress_cb, NULL);
        }
        
        free(src_path);
        free(dst_path);
        
        if (ret != 0) {
            ui_message("Error", "Failed to copy file");
            return -1;
        }
    }
    
    panel_refresh(dst);
    return 0;
}

int fileops_move(Panel *src, Panel *dst) {
    DirEntry *e = &src->entries[src->cursor];
    
    char *src_path = pl_path_join(src->path, e->name);
    char *dst_path = pl_path_join(dst->path, e->name);
    
    ui_draw_status("Moving...");
    int ret = pl_move(src_path, dst_path);
    
    free(src_path);
    free(dst_path);
    
    if (ret != 0) {
        ui_message("Error", "Failed to move file");
        return -1;
    }
    
    panel_refresh(src);
    panel_refresh(dst);
    return 0;
}

int fileops_delete(Panel *p) {
    int selected = panel_get_selected_count(p);
    int total = (selected > 0) ? selected : 1;
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Delete %d item(s)?", total);
    
    if (!ui_confirm(msg)) return -1;
    
    for (int i = 0; i < p->count; i++) {
        DirEntry *e = &p->entries[i];
        if (!e->selected && i != p->cursor) continue;
        
        char *path = pl_path_join(p->path, e->name);
        ui_draw_status("Deleting...");
        pl_delete(path);
        free(path);
    }
    
    panel_refresh(p);
    return 0;
}

int fileops_mkdir(Panel *p, const char *name) {
    char *path = pl_path_join(p->path, name);
    int ret = pl_mkdir(path);
    free(path);
    
    if (ret != 0) {
        ui_message("Error", "Failed to create directory");
        return -1;
    }
    
    panel_refresh(p);
    return 0;
}

int fileops_rename(Panel *p, const char *newname) {
    DirEntry *e = &p->entries[p->cursor];
    char *old_path = pl_path_join(p->path, e->name);
    char *new_path = pl_path_join(p->path, newname);
    
    int ret = pl_move(old_path, new_path);
    
    free(old_path);
    free(new_path);
    
    if (ret != 0) {
        ui_message("Error", "Failed to rename");
        return -1;
    }
    
    panel_refresh(p);
    return 0;
}
