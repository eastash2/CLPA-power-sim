#include "../include/page.h"
extern count_t cur_tick;
extern int verbose;
page_p get_swap_candidate(page_p clp) {
    page_p page;
    for(page = clp->next; page != clp; page = page->next)
        if(page->expiration <= cur_tick && page->swapper == NULL)
            return page;
    
    return NULL;
}

page_p add_new_page(page_p head, uint64_t pn) {
    page_p new_page = malloc(sizeof(page_t));

    new_page->page_number = pn;
    new_page->counter = 0;
    new_page->next = head;
    new_page->prev = head->prev;
    new_page->swap_tick = 0;
    new_page->expiration = cur_tick + RT_LIFETIME;
    new_page->swapper = NULL;
    new_page->prev->next = new_page;
    head->prev = new_page;

    return new_page;
}

page_p get_page(page_p head, uint64_t pn) {
    page_p page;
    for(page = head->next; page != head; page = page->next) {
        if(page->page_number == pn)
            return page;
    }
    return NULL;
}


page_p init_head(void) {
    page_p head = malloc(sizeof(page_t));
    head->next = head;
    head->prev = head;
    return head;
}

void delete_page(page_p page) {
    page->prev->next = page->next;
    page->next->prev = page->prev;
    free(page);

}
void move_page(page_p page, page_p head) {
    page->prev->next = page->next;
    page->next->prev = page->prev;

    page->next = head->next;
    page->prev = head;

    page->next->prev = page;
    head->next = page;
    return;
}
