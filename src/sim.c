#include "../include/sim.h"
static int verbose;

void set_verbose(int v) {verbose = v;}
//depends trace formats
//use "access_tick address R/W" format
trace_t parse_line(char* buf) {
    trace_t tr;
    char* p;
    p = strchr(buf, ' ');
    *p = '\0';
    tr.tick = strtol(buf, NULL, 10);
    buf = p + 1;
    tr.address = strtol(buf, NULL, 16);
    p = strchr(buf, ' ');
    buf = p + 1;
    tr.rw = *buf == 'R' ? ACCESS_READ :
         *buf == 'W' ? ACCESS_WRITE :
                         ACCESS_INVAL;

    p = strchr(buf, ' ');
    *p = '\0';
    buf = p + 1;
    tr.size = strtol(buf, NULL, 10);
    return tr;
}

void timer_increase(page_p head, long int tick) {
    page_p page;
    for(page = head->next; page != head; page = page->next) {
        page->timer += tick;
        if(page->swapper != NULL) {
            page->swap_left -= page->swap_left < tick ? page->swap_left : tick;            
        }
    }
    return;
}


void page_check(page_p rt, page_p clp) {
    page_p page;
    page_p next;
    page_p swap;

    page = rt->next; 
    while(page != rt) {
        next = page->next;
        if(page->timer >= RT_LIFETIME && page->swapper == NULL) {
            delete_page(page);
            rt_length--;
        } else if(page->counter >= hot_threshold && page->swapper == NULL) {
            swaps_count++;
            if((swap = get_swap_candidate(clp)) != NULL) {
                if(verbose) {
                    printf("%lu: page %lx will be swapped to CLP!\n", 
                    cur_tick, page->page_number);
                    printf("%lu: page %lx will be swapped to RT!\n", 
                    cur_tick, swap->page_number);
                }
                swap->swap_left = SWAP_LATENCY;
                swap->swapper = page;
                page->swap_left = SWAP_LATENCY;
                page->swapper = swap;
            } else {
                if(verbose)
                    printf("%lu: page %lx will be moved to CLP!\n", 
                        cur_tick, page->page_number);
                page->swap_left = SWAP_LATENCY;
                page->swapper = clp;
            }
        }
        if(page->swapper != NULL && page->swap_left == 0) {
            if(verbose)
                printf("%lu: page %lx swapped to CLP!\n", 
                cur_tick, page->page_number);

            page->timer = 0;
            page->counter = 0;
            move_page(page, clp);
            if(page->swapper != clp) {
                if(verbose)
                    printf("%lu: page %lx swapped to RT!\n", 
                    cur_tick, page->swapper->page_number);
                page->swapper->timer = 0;
                page->swapper->counter = 0;
                move_page(page->swapper, rt);
                page->swapper->swapper = NULL;
            }
            else {
                clp_length++;
                rt_length--;
            }
            page->swapper = NULL;

        }
            
        page = next;
    }
    return;
}

void access_page(page_p rt, page_p clp, trace_t tr) {
    page_p page;
    uint64_t pn_start, pn_end;
    uint64_t pn;
    pn_start = tr.address >> PAGE_SIZE;
    pn_end = (tr.address + tr.size) >> PAGE_SIZE;
    pn_end++;
    for(pn = pn_start; pn < pn_end; pn++) {
        page = get_page(clp, pn);
        if(page == NULL) {
            rt_count++;
            page = get_page(rt, pn);
            if(page == NULL) {
                page = add_new_page(rt, pn);
                rt_length++;
            }
        }
        else {
            clp_count++;
        }
        page->counter++;
        page->timer = 0;
    }

    return;
}


