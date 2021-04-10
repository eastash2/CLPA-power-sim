#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BUF_SIZE 64
#define PAGE_SIZE 12
#define ACCESS_READ 0
#define ACCESS_WRITE 1
#define ACCESS_MODIFY 2
#define ACCESS_INVAL -1

#define RT_LIFETIME 2000
#define CLP_LIFETIME 2000
#define HOT_THRESHOLD 20


typedef struct _sim_page {
    uint64_t page_number;
    unsigned long counter;
    unsigned long timer;
    struct _sim_page *prev, *next;
} page_t;

static page_t* add_new_page(page_t* head, uint64_t pn) {
    page_t* new_page = malloc(sizeof(page_t));

    new_page->page_number = pn;
    new_page->counter = 0;
    new_page->next = head;
    new_page->prev = head->prev;

    new_page->prev->next = new_page;
    head->prev = new_page;

    return new_page;
}

static page_t* get_page(page_t* head, uint64_t pn) {
    page_t* page;
    for(page = head->next; page != head; page = page->next)
        if(page->page_number == pn)
            return page;


    return NULL;
}

//depends trace formats
//temporarily use CSAPP cachelab trace format
static void parse_line(char* buf, uint64_t* page_num, int* rw) {
    buf[2] = '\0';
    *rw = !strcmp(buf, " L") 
            || !strcmp(buf, " I") ? ACCESS_READ :
                !strcmp(buf, " S") ? ACCESS_WRITE :
                !strcmp(buf, " M") ? ACCESS_MODIFY :
                                    ACCESS_INVAL;
    *page_num = strtol(&buf[3], NULL, 16) >> PAGE_SIZE;

    return;
}

static void access_page(page_t* rt, page_t* clp, uint64_t pn, int rw) {
    page_t* page;
    page = get_page(clp, pn);
    if(page == NULL)
        page = get_page(rt, pn);
    if(page == NULL)
        page = add_new_page(rt, pn);

    page->counter++;
    page->timer = 0;

    return;
}

static void print_result(page_t* head) {
    page_t* page;
    for(page = head->next; page != head; page = page->next) {
        printf("PN %lx: %ld times accesses\n", page->page_number, page->counter);
    }

    return;
}

static page_t* init_head(void) {
    page_t* head = malloc(sizeof(page_t));
    head->next = head;
    head->prev = head;
    return head;
}


static void timer_increase(page_t* head) {
    page_t* page;
    for(page = head->next; page != head; page = page->next) {
        page->timer++;
    }
    return;
}

static void move_page(page_t* page, page_t* head) {
    page->prev->next = page->next;
    page->next->prev = page->prev;

    page->next = head;
    page->prev = head->prev;

    page->prev->next = page;
    head->prev = page;
    return;
}

static void page_check(page_t* rt, page_t* clp) {
    page_t* page;
    page_t* next;

    page = clp->next; 
    while(page != clp) {
        next = page->next;
        if(page->timer >= CLP_LIFETIME) {
            page->timer = 0;
            move_page(page, rt);
        }         
        page = next;        
    }

    page = rt->next; 
    while(page != rt) {
        next = page->next;
        if(page->timer >= RT_LIFETIME) {
            page->timer = 0;
            page->counter = 0;
        } else if(page->counter >= HOT_THRESHOLD) {
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
    uint64_t page_num;
    page_t* rt_pages_head;
    page_t* clp_pages_head;
    int rw;

    if(argc != 2) {
        printf("usage: sim {memory trace}\n");
        return 0;
    }

    rt_pages_head = init_head();
    clp_pages_head = init_head();
    fp = fopen(argv[1], "r");
    while(fgets(buf, BUF_SIZE, fp) != NULL) {
        timer_increase(rt_pages_head);
        timer_increase(clp_pages_head);
        parse_line(buf, &page_num, &rw);
        access_page(rt_pages_head, clp_pages_head, page_num, rw);
        page_check(rt_pages_head, clp_pages_head);
    }


    printf("rt lists:\n");
    print_result(rt_pages_head);
    printf("clp lists:\n");
    print_result(clp_pages_head);

    fclose(fp);

    return 0;
}
