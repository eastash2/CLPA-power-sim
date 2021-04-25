#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define ACCESS_READ 0
#define ACCESS_WRITE 1
#define ACCESS_INVAL -1
#define RT_LIFETIME 200e6
#define CLP_LIFETIME 200e6

#define PAGE_SIZE 512
typedef uint64_t count_t;

typedef struct _sim_page {
    uint64_t page_number;
    count_t counter;
    count_t expiration;
    count_t swap_tick;
    struct _sim_page *prev, *next;
    struct _sim_page *swapper;
} page_t;

typedef page_t* page_p;

page_p get_swap_candidate(page_p clp); 
page_p add_new_page(page_p head, uint64_t pn);
page_p get_page(page_p head, uint64_t pn);
page_p init_head(void); 
void delete_page(page_p page);
void move_page(page_p page, page_p head);
