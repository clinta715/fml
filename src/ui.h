#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include "fml.h"

#define UI_MIN_WIDTH 80
#define UI_MIN_HEIGHT 24

typedef struct {
    int width;
    int height;
    int panel_width;
    int preview_height;
} UILayout;

int ui_init(void);
void ui_cleanup(void);
void ui_resize(void);
void ui_draw(void);
void ui_draw_panel(Panel *p, int x, int y, int w, int h, bool active);
void ui_draw_preview(Panel *p, int x, int y, int w, int h);
void ui_draw_status(const char *msg);
void ui_draw_help(void);
int ui_get_key(void);
void ui_beep(void);
void ui_message(const char *title, const char *msg);
char *ui_input(const char *prompt, const char *initial);
int ui_confirm(const char *msg);
UILayout ui_get_layout(void);
int ui_progress(const char *title, const char *filename, uint64_t done, uint64_t total);
int ui_check_cancel(void);
void ui_set_cancel(int cancel);

#endif