#include "preview.h"
#include "platform.h"
#include <ctype.h>

#define PREVIEW_MAX_BYTES 65536

static void hex_dump(const unsigned char *data, size_t len, char *out, int max_lines, int width) {
    (void)width;
    int pos = 0;
    int line = 0;
    while ((size_t)pos < len && line < max_lines) {
        int row_start = pos;
        out += sprintf(out, "%08x: ", pos);
        
        for (int i = 0; i < 16 && (size_t)pos < len; i++) {
            out += sprintf(out, "%02x ", (unsigned char)data[pos++]);
        }
        
        out += sprintf(out, " |");
        for (int i = row_start; (size_t)i < (size_t)pos; i++) {
            *out++ = isprint(data[i]) ? data[i] : '.';
        }
        *out++ = '|';
        *out++ = '\n';
        line++;
    }
    *out = '\0';
}

char *preview_get(const char *path, int width, int max_lines) {
    if (pl_is_dir(path)) {
        char *buf = xmalloc(256);
        snprintf(buf, 256, "Directory: %s", path);
        return buf;
    }
    
    size_t len;
    char *content = pl_read_file(path, PREVIEW_MAX_BYTES, &len);
    if (!content) {
        char *buf = xmalloc(256);
        snprintf(buf, 256, "Cannot read file");
        return buf;
    }
    
    if (pl_is_text_file(path)) {
        int lines = 0;
        char *p = content;
        while (*p && lines < max_lines) {
            if (*p == '\n') lines++;
            p++;
        }
        *p = '\0';
        (void)width;
        return content;
    }
    
    char *hex_buf = xmalloc((size_t)(max_lines * (width + 32)));
    hex_dump((unsigned char *)content, len, hex_buf, max_lines, width);
    free(content);
    
    return hex_buf;
}