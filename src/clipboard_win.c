#ifdef _WIN32

#include "clipboard.h"
#include <windows.h>

int clipboard_copy_text(const char *text) {
    if (!text || !OpenClipboard(NULL)) return -1;
    
    EmptyClipboard();
    
    size_t len = strlen(text) + 1;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(WCHAR));
    if (!h) {
        CloseClipboard();
        return -1;
    }
    
    WCHAR *wdata = GlobalLock(h);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wdata, wlen);
    GlobalUnlock(h);
    
    SetClipboardData(CF_UNICODETEXT, h);
    CloseClipboard();
    return 0;
}

char *clipboard_paste_text(void) {
    if (!OpenClipboard(NULL)) return NULL;
    
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (!h) {
        CloseClipboard();
        return NULL;
    }
    
    WCHAR *wdata = GlobalLock(h);
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wdata, -1, NULL, 0, NULL, NULL);
    char *result = malloc(utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, wdata, -1, result, utf8_len, NULL, NULL);
    GlobalUnlock(h);
    CloseClipboard();
    
    return result;
}

#endif
