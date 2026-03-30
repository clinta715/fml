#ifndef SEARCH_H
#define SEARCH_H

#include "fml.h"

void search_start(Panel *p);
void search_update(Panel *p, char ch);
void search_finish(Panel *p);
void search_cancel(Panel *p);

#endif