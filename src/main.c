#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define FLAG_HOT 0b1
#define FLAG_IN_CLP 0b01

#define BUF_SIZE 64
#define PAGE_SIZE 12
#define ACCESS_READ 0
#define ACCESS_WRITE 1
#define ACCESS_MODIFY 2
#define ACCESS_INVAL -1

typedef struct _sim_page {
    uint64_t page_number;
    uint64_t last_access;
    int flags;
    struct _sim_page *next;
} page_t;

//depends trace formats
//temporarily use CSAPP cachelab trace format
void parse_line(char* buf, uint64_t* page_num, int* rw) {
    buf[2] = '\0';
    *rw = !strcmp(buf, " L") 
            || !strcmp(buf, " I") ? ACCESS_READ :
                !strcmp(buf, " S") ? ACCESS_WRITE :
                !strcmp(buf, " M") ? ACCESS_MODIFY :
                                    ACCESS_INVAL;
    *page_num = strtol(&buf[3], NULL, 16) >> PAGE_SIZE;

    return;
}

void simulate(page_t* page_list, uint64_t page_num, int rw) {
    printf("accessing page %ld, with rw bit %d\n", page_num, rw);
    return;
}

int main(int argc, char* argv[]) {
    FILE* fp;
    char buf[BUF_SIZE];
    uint64_t page_num;
    page_t* page_list; 
    int rw;

    if(argc != 2) {
        printf("usage: sim {memory trace}\n");
        return 0;
    }

    page_list = NULL;

    fp = fopen(argv[1], "r");
    while(fgets(buf, BUF_SIZE, fp) != NULL) {
        parse_line(buf, &page_num, &rw);
        simulate(page_list, page_num, rw);
    }
    fclose(fp);


    return 0;
}
