#ifndef _WIN32

#include "clipboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *clipboard_buf = NULL;

int clipboard_copy_text(const char *text) {
    if (!text) return -1;
    
    FILE *p = popen("pbcopy 2>/dev/null", "w");
    if (!p) {
        p = popen("xclip -selection clipboard 2>/dev/null", "w");
    }
    if (!p) return -1;
    
    size_t len = strlen(text);
    size_t written = fwrite(text, 1, len, p);
    pclose(p);
    
    return (written == len) ? 0 : -1;
}

char *clipboard_paste_text(void) {
    FILE *p = popen("pbpaste 2>/dev/null", "r");
    if (!p) {
        p = popen("xclip -selection clipboard -o 2>/dev/null", "r");
    }
    if (!p) return NULL;
    
    char *buf = malloc(4096);
    if (!buf) {
        pclose(p);
        return NULL;
    }
    
    size_t len = fread(buf, 1, 4095, p);
    buf[len] = '\0';
    pclose(p);
    
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    
    return buf;
}

#endif
