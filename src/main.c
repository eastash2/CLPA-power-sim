#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#define BUF_SIZE 64
#define PAGE_SIZE 9

#define ACCESS_READ 0
#define ACCESS_WRITE 1
#define ACCESS_INVAL -1

#define SIM_TICK 1

#define RT_LIFETIME 200e6
#define CLP_LIFETIME 200e6

#define NUM_CHIPS 12

#define RT_PJ_PER_ACCESS 2000
#define RT_STATIC_MILIWATT 171
#define RT_CHIPS 0.93 * NUM_CHIPS

#define CLP_PJ_PER_ACCESS 510
#define CLP_STATIC_MILIWATT 1.29
#define CLP_CHIPS 0.07 * NUM_CHIPS

#define SWAP_LATENCY 1200000

typedef unsigned long count_t;

typedef struct _sim_page {
    uint64_t page_number;
    count_t counter;
    count_t timer;
    struct _sim_page *prev, *next;
} page_t;

typedef page_t* page_p;
typedef struct _sim_trace {
    uint64_t address;
    size_t size;
    int rw;
    count_t tick;
} trace_t;

static count_t cur_tick;
static count_t clp_count;
static count_t rt_count;
static count_t swaps_count;
static count_t hot_threshold;
static int verbose;

static page_p get_swap_candidate(page_p clp) {
    page_p page;
    for(page = clp->next; page != clp; page = page->next)
        if(page->timer >= CLP_LIFETIME)
            return page;
    
    return NULL;
}

static page_p add_new_page(page_p head, uint64_t pn) {
    page_p new_page = malloc(sizeof(page_t));

    new_page->page_number = pn;
    new_page->counter = 0;
    new_page->next = head;
    new_page->prev = head->prev;

    new_page->prev->next = new_page;
    head->prev = new_page;

    return new_page;
}

static page_p get_page(page_p head, uint64_t pn) {
    page_p page;
    for(page = head->next; page != head; page = page->next)
        if(page->page_number == pn)
            return page;


    return NULL;
}

//depends trace formats
//use "access_tick address R/W" format
static trace_t parse_line(char* buf) {
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

static void access_page(page_p rt, page_p clp, trace_t tr) {
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
            if(page == NULL)
                page = add_new_page(rt, pn);
        }
        else {
            clp_count++;
        }
        page->counter++;
        page->timer = 0;
    }

    return;
}

static void print_result(page_p head) {
    page_p page;
    for(page = head->next; page != head; page = page->next) {
        printf("PN %lx: %ld times accesses\n", page->page_number, page->counter);
    }

    return;
}

static page_p init_head(void) {
    page_p head = malloc(sizeof(page_t));
    head->next = head;
    head->prev = head;
    return head;
}


static void timer_increase(page_p head, long int tick) {
    page_p page;
    for(page = head->next; page != head; page = page->next) {
        page->timer += tick;
    }
    return;
}

static void move_page(page_p page, page_p head) {
    page->prev->next = page->next;
    page->next->prev = page->prev;

    page->next = head;
    page->prev = head->prev;

    page->prev->next = page;
    head->prev = page;
    return;
}

static void page_check(page_p rt, page_p clp) {
    page_p page;
    page_p next;
    page_p swap;

    page = rt->next; 
    while(page != rt) {
        next = page->next;
        if(page->timer >= RT_LIFETIME) {
            page->timer = 0;
            page->counter = 0;
        } else if(page->counter >= hot_threshold) {
            swaps_count++;
            if((swap = get_swap_candidate(clp)) != NULL) {
                if(verbose) {
                    printf("%lu: page %lx swapped to CLP!\n", 
                    cur_tick, page->page_number);
                    printf("%lu: page %lx swapped to RT!\n", 
                    cur_tick, swap->page_number);
                }
                swap->counter = 0;
                swap->timer = 0;
                move_page(swap, rt);
            } else {
                if(verbose)
                    printf("%lu: page %lx moved to CLP!\n", 
                        cur_tick, page->page_number);
            }
            page->counter = 0;
            page->timer = 0;
            move_page(page, clp);            
        }
        page = next;
    }
    return;
}

int main(int argc, char* argv[]) {
    FILE* fp;
    char buf[BUF_SIZE];
    count_t new_tick, sim_tick;
    page_p rt_pages_head, clp_pages_head;
    trace_t tr;
    double sec, clpa_pJ, conv_pJ, swap_penalty;
    double clpa_dp, clpa_sp, conv_dp, conv_sp;

    struct timeval start, end;

    if(argc < 3) {
        printf("usage: sim \"hotpage threshold\" \"memory trace\" {--verbose}\n");
        return 0;
    }
    hot_threshold = atoi(argv[1]);
    if(argc >= 4)
        verbose = !strcmp("--verbose", argv[3]);
    else
        verbose = 0;

    gettimeofday(&start, 0);

    rt_pages_head = init_head();
    clp_pages_head = init_head();
    clp_count = 0;
    rt_count = 0;
    cur_tick = 0;
    fp = fopen(argv[2], "r");
    while(fgets(buf, BUF_SIZE, fp) != NULL) {
        tr = parse_line(buf);
        new_tick = tr.tick;
        while(cur_tick != new_tick) {
            sim_tick = new_tick - cur_tick;
            timer_increase(rt_pages_head, sim_tick);
            timer_increase(clp_pages_head, sim_tick);
            page_check(rt_pages_head, clp_pages_head);            
            cur_tick += sim_tick;
        }
        access_page(rt_pages_head, clp_pages_head, tr);
        page_check(rt_pages_head, clp_pages_head);
    }

    gettimeofday(&end, 0);
    clpa_pJ = clp_count * CLP_PJ_PER_ACCESS + 
              rt_count * RT_PJ_PER_ACCESS + 
              swaps_count * ((CLP_PJ_PER_ACCESS + RT_PJ_PER_ACCESS) << 3);
    conv_pJ = (clp_count + rt_count) * RT_PJ_PER_ACCESS;
    sec = (double)cur_tick / 1e12;
    swap_penalty = swaps_count * SWAP_LATENCY / 1e12;
    printf("workload time: %lf secs\n", sec + swap_penalty);
    printf("simulation time: %lf secs\n", 
            ((double)end.tv_sec - start.tv_sec 
             + (end.tv_usec - start.tv_usec) * 1e-6));
    printf("clp access: %lu, rt access: %lu\n", clp_count, rt_count);
    printf("swap count: %lu\n", swaps_count);

    clpa_dp = (clpa_pJ / (sec + swap_penalty)) * 1e-9;
    clpa_sp = CLP_STATIC_MILIWATT * CLP_CHIPS + RT_STATIC_MILIWATT * RT_CHIPS;
    printf("CLP-A dynamic energy: %.3lf pJ\n", clpa_pJ);
    printf("CLP-A static energy: %.3lf pJ\n", 
            clpa_sp * (sec + swap_penalty) * 1e9);
    printf("CLP-A total energy: %.3lf mJ\n", 
            clpa_pJ * 1e-9 + clpa_sp * (sec + swap_penalty));
    printf("CLP-A total power: %.3lf mW\n", clpa_dp + clpa_sp);
    conv_dp = (conv_pJ / sec) * 1e-9;
    conv_sp = RT_STATIC_MILIWATT * NUM_CHIPS;
    printf("convention dynamic energy: %.3lf pJ\n", conv_pJ);
    printf("convention static energy: %.3lf pJ\n", conv_sp * sec * 1e9);
    printf("convention total energy: %.3lf mJ\n", conv_pJ * 1e-9 + conv_sp * sec);
    printf("convention total power: %.3lf mW\n", conv_dp + conv_sp);

    printf("normalized power: %.3lf%% \n", 
            (clpa_dp + clpa_sp) / (conv_dp + conv_sp) * 100);
    fclose(fp);

    return 0;
}
