#ifdef _WIN32

#include "clipboard.h"
#include <windows.h>

int clipboard_copy_text(const char *text) {
    if (!text || !OpenClipboard(NULL)) return -1;
    
    EmptyClipboard();
    
    size_t len = strlen(text) + 1;
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, len);
    if (!h) {
        CloseClipboard();
        return -1;
    }
    
    char *data = GlobalLock(h);
    memcpy(data, text, len);
    GlobalUnlock(h);
    
    SetClipboardData(CF_TEXT, h);
    CloseClipboard();
    return 0;
}

char *clipboard_paste_text(void) {
    if (!OpenClipboard(NULL)) return NULL;
    
    HANDLE h = GetClipboardData(CF_TEXT);
    if (!h) {
        CloseClipboard();
        return NULL;
    }
    
    char *data = GlobalLock(h);
    char *result = strdup(data);
    GlobalUnlock(h);
    CloseClipboard();
    
    return result;
}

#endif
