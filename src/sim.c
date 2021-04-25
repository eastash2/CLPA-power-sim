#include "../include/sim.h"
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


void page_check(page_p rt, page_p clp) {
    page_p page;
    page_p next;
    page_p swap;

    page = rt->next; 
    while(page != rt) {
        next = page->next;
        if(page->expiration <= cur_tick && page->swapper == NULL) {
            delete_page(page);
            rt_length--;
        } else if(page->counter >= hot_threshold && page->swapper == NULL) {
            swaps_count++;
            if((swap = get_swap_candidate(clp)) != NULL) {
                if(verbose > 0) {
                    printf("%lu: page %lx will be swapped to CLP!\n", 
                    cur_tick, page->page_number);;
                    printf("%lu: page %lx will be swapped to RT!\n", 
                    cur_tick, swap->page_number);
                }
                swap->swap_tick = cur_tick + SWAP_LATENCY;
                swap->swapper = page;
                page->swap_tick = cur_tick + SWAP_LATENCY;
                page->swapper = swap;
            } else {
                if(verbose > 0)
                    printf("%lu: page %lx will be moved to CLP!\n", 
                        cur_tick, page->page_number);
                page->swap_tick = cur_tick + SWAP_LATENCY;
                page->swapper = clp;
            }
        }
        if(page->swapper != NULL && page->swap_tick <= cur_tick) {
            if(verbose > 0)
                printf("%lu: page %lx swapped to CLP!\n", 
                cur_tick, page->page_number);

            page->expiration = cur_tick + CLP_LIFETIME;
            page->counter = 0;
            move_page(page, clp);
            clp_length++;
            rt_length--;
            if(page->swapper != clp) {
                delete_page(page->swapper);
                clp_length--;
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
    pn_start = tr.address / PAGE_SIZE;
    pn_end = (tr.address + tr.size) / PAGE_SIZE;
    pn_end++;
    for(pn = pn_start; pn < pn_end; pn++) {
        if(verbose > 0)
            printf("%lu: finding page %lx in CLP..\n", cur_tick, pn);
        page = get_page(clp, pn);
        if(page == NULL) {
            if(verbose > 0)
                printf("%lu: finding page %lx in RT..\n", cur_tick, pn);
            rt_count++;
            page = get_page(rt, pn);
            if(page == NULL) {
                if(verbose > 0)
                    printf("%lu: page %lx not found.. create in RT\n", cur_tick, pn);
                page = add_new_page(rt, pn);
                rt_length++;
            }
            page->expiration = cur_tick + RT_LIFETIME;
        }
        else {
            move_page(page, clp);
            page->expiration = cur_tick + CLP_LIFETIME;
            clp_count++;
        }
        page->counter++;
    }

    return;
}


