#include "../include/sim.h"
#include <sys/time.h>
#define BUF_SIZE 128

#define SIM_TICK 1.2e6
#define PRINT_TICK 1e6

int main(int argc, char* argv[]) {
    FILE* fp;
    char buf[BUF_SIZE];
    count_t new_tick, sim_tick;
    page_p rt_pages_head, clp_pages_head;
    trace_t tr;
    double sec, clpa_nJ, conv_nJ;
    double clpa_dp, clpa_sp, conv_dp, conv_sp;
    count_t print_tick;
    struct timeval start, end;
    count_t initial_tick;

    if(argc < 3) {
        printf("usage: sim \"hotpage threshold\" \"memory trace\" {--verbose}\n");
        return 0;
    }
    hot_threshold = atoi(argv[1]);
    if(argc >= 4) {
        if(!strcmp("--verbose", argv[3]))
            set_verbose(1);
        else if(!strcmp("--silent", argv[3]))
            set_verbose(-1);
    }
    else
        set_verbose(0);

    gettimeofday(&start, 0);

    rt_pages_head = init_head();
    clp_pages_head = init_head();
    clp_count = 0;
    rt_count = 0;
    cur_tick = 0, initial_tick = 0;
    print_tick = 0;
    fp = fopen(argv[2], "r");
    while(fgets(buf, BUF_SIZE, fp) != NULL) {
        tr = parse_line(buf);
        new_tick = tr.tick;
        if(initial_tick == 0)
            initial_tick = tr.tick;
        if(new_tick < cur_tick) {
            printf("unsorted trace!\n");
            return 0;
        }
        while(1) {
            cur_tick += SIM_TICK;
            if(new_tick <= cur_tick)
                break;
            page_check(rt_pages_head, clp_pages_head);
        }
        cur_tick = new_tick;
        if(verbose > 0)
            printf("%lu: try access page %lx..\n", cur_tick, tr.address / PAGE_SIZE);
        access_page(rt_pages_head, clp_pages_head, tr);
        page_check(rt_pages_head, clp_pages_head);
        if(print_tick * PRINT_TICK < cur_tick && verbose > -1) {
            printf("simulated tick: %lu, rt hit: %lu, clp hit: %lu, rt length: %lu, clp length: %lu       \r",
                    cur_tick - initial_tick, rt_count, clp_count, rt_length, clp_length);
            print_tick = cur_tick / PRINT_TICK + 1;
        }
    }


    gettimeofday(&end, 0);
    printf("\n");
    clpa_nJ = clp_count * CLP_NJ_PER_ACCESS + 
              rt_count * RT_NJ_PER_ACCESS + 
              swaps_count * ((CLP_NJ_PER_ACCESS + RT_NJ_PER_ACCESS) * 8);
    conv_nJ = (clp_count + rt_count) * RT_NJ_PER_ACCESS;
    sec = (double)(cur_tick - initial_tick) / 1e12;
    printf("workload time: %lf secs\n", sec);
    printf("simulation time: %lf secs\n", 
            ((double)end.tv_sec - start.tv_sec 
             + (end.tv_usec - start.tv_usec) * 1e-6));
    printf("clp access: %lu, rt access: %lu\n", clp_count, rt_count);
    printf("swap count: %lu\n", swaps_count);

    clpa_dp = (clpa_nJ / sec) * 1e-6;
    clpa_sp = CLP_STATIC_MILIWATT * CLP_CHIPS + RT_STATIC_MILIWATT * RT_CHIPS;
    printf("CLP-A dynamic energy: %.3lf nJ\n", clpa_nJ);
    printf("CLP-A static energy: %.3lf nJ\n", 
            clpa_sp * sec * 1e6);
    printf("CLP-A total energy: %.3lf mJ\n", 
            clpa_nJ * 1e-6 + clpa_sp * sec );
    printf("CLP-A total power: %.3lf mW\n", clpa_dp + clpa_sp);
    conv_dp = (conv_nJ / sec) * 1e-6;
    conv_sp = RT_STATIC_MILIWATT * NUM_CHIPS;
    printf("convention dynamic energy: %.3lf nJ\n", conv_nJ);
    printf("convention static energy: %.3lf nJ\n", conv_sp * sec * 1e6);
    printf("convention total energy: %.3lf mJ\n", conv_nJ * 1e-6 + conv_sp * sec);
    printf("convention total power: %.3lf mW\n", conv_dp + conv_sp);

    printf("normalized power: %.3lf%% \n", 
            (clpa_dp + clpa_sp) / (conv_dp + conv_sp) * 100);
    fclose(fp);

    return 0;
}
