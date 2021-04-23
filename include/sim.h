#include "page.h"
#include <string.h>
#include <stdio.h>


#define NUM_CHIPS 8
#define RT_NJ_PER_ACCESS 2
#define RT_STATIC_MILIWATT 171
#define RT_CHIPS 0.93 * NUM_CHIPS

#define CLP_NJ_PER_ACCESS 0.51
#define CLP_STATIC_MILIWATT 1.29
#define CLP_CHIPS 0.07 * NUM_CHIPS

#define SWAP_LATENCY 1.2e6
typedef struct _sim_trace {
    uint64_t address;
    size_t size;
    int rw;
    count_t tick;
} trace_t;

void set_verbose(int v);
count_t cur_tick;
count_t clp_count;
count_t rt_count;
count_t swaps_count;
count_t hot_threshold;
count_t clp_length;
count_t rt_length;

trace_t parse_line(char* buf);
void timer_increase(page_p head, long int tick);
void page_check(page_p rt, page_p clp); 
void access_page(page_p rt, page_p clp, trace_t tr) ;
