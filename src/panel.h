#ifndef PANEL_H
#define PANEL_H

#include "fml.h"

int panel_init(Panel *p, const char *path);
void panel_free(Panel *p);
int panel_refresh(Panel *p);
int panel_cursor_up(Panel *p);
int panel_cursor_down(Panel *p);
int panel_enter(Panel *p);
int panel_parent(Panel *p);
int panel_goto(Panel *p, const char *path);
void panel_apply_filter(Panel *p);
int panel_get_selected_count(Panel *p);
void panel_select_all(Panel *p, bool select);
void panel_select_pattern(Panel *p, const char *pattern, bool select);
uint64_t panel_get_total_size(Panel *p);
uint64_t panel_get_selected_size(Panel *p);
void panel_cycle_sort(Panel *p);
void panel_toggle_sort_reverse(Panel *p);
const char* panel_get_sort_name(Panel *p);

#endif