#include "search.h"
#include "panel.h"
#include <string.h>
#include <ctype.h>

static int fuzzy_match_score(const char *name, const char *pattern) {
    if (pattern[0] == '\0') return 0;
    
    size_t plen = strlen(pattern);
    size_t nlen = strlen(name);
    
    if (nlen < plen) return -1;
    
    size_t pi = 0;
    int score = 0;
    int consecutive = 0;
    
    for (size_t i = 0; i < nlen && pi < plen; i++) {
        if (tolower((unsigned char)name[i]) == tolower((unsigned char)pattern[pi])) {
            pi++;
            consecutive++;
            score += 10 + consecutive * 5;
            if (i == 0 || name[i-1] == '_' || name[i-1] == '-' || name[i-1] == '.') {
                score += 15;
            }
            if (isupper((unsigned char)name[i]) && (i == 0 || islower((unsigned char)name[i-1]))) {
                score += 10;
            }
        } else {
            consecutive = 0;
        }
    }
    
    return (pi == plen) ? score : -1;
}

void search_start(Panel *p) {
    (void)p;
    g_state.mode = MODE_SEARCH;
    g_state.input_len = 0;
    g_state.input_buffer[0] = '\0';
}

void search_update(Panel *p, char ch) {
    if (g_state.input_len < MAX_SEARCH - 1) {
        g_state.input_buffer[g_state.input_len++] = ch;
        g_state.input_buffer[g_state.input_len] = '\0';
        
        strncpy(p->search_filter, g_state.input_buffer, MAX_SEARCH - 1);
        panel_refresh(p);
        
        if (g_state.input_len > 0) {
            int best_score = -1;
            int best_idx = -1;
            
            for (int i = 0; i < p->count; i++) {
                int score = fuzzy_match_score(p->entries[i].name, g_state.input_buffer);
                if (score > best_score) {
                    best_score = score;
                    best_idx = i;
                }
            }
            
            if (best_idx >= 0) {
                p->cursor = best_idx;
            }
        }
    }
}

void search_finish(Panel *p) {
    (void)p;
    g_state.mode = MODE_NORMAL;
}

void search_cancel(Panel *p) {
    p->search_filter[0] = '\0';
    panel_refresh(p);
    g_state.mode = MODE_NORMAL;
}