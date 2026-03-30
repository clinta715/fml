#ifndef CLIPBOARD_H
#define CLIPBOARD_H

int clipboard_copy_text(const char *text);
char *clipboard_paste_text(void);

#endif
