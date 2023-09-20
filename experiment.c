#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>


#include "sbmem.h"

#define PROCESS_COUNT 4
#define ALLOC_COUNT 8
#define SEG_SIZE 32768

// for gaussian distribution of allocation sizes
#define SIGMA 200.0
#define MU 400.0

int gaussian_random() 
{
   double no1 = ( (double) (rand()) + 1. ) / ( (double) (RAND_MAX) + 1. );
   double no2 = ( (double) (rand()) + 1. ) / ( (double) (RAND_MAX) + 1. );
   return (int) ( MU + SIGMA * sqrt( log(no1) * -2. ) * cos( 3.14 * no2 * 2 ) );
}

void process_func() {
    sbmem_open();
    void *ps[ALLOC_COUNT];

    int i;
    int alloc_size;

    void *p;
    for (i = 0; i < ALLOC_COUNT; i++) {
        alloc_size = gaussian_random();
        if (alloc_size < 128) {
            alloc_size = 128;
        }
        if (alloc_size > 4096) {
            alloc_size = 4096;
        }
        //printf("%d\n", alloc_size);
        p = sbmem_alloc(alloc_size);
        ps[i] = p;
    }

    /*
    for (i = 0; i < ALLOC_COUNT; i++) {
        sbmem_free(ps[i]);
    }
    */

    // sbmem_close();
    exit(0);
}

int main() {
    srand(111);

    sbmem_init(SEG_SIZE);
    
    int i;
    for (i = 0; i < PROCESS_COUNT; i++) {
        if (fork() == 0) {
            process_func();
        }
    }

    for (i = 0; i < PROCESS_COUNT; i++) {
        wait(NULL);
    }

    sbmem_open();

    //print_tree();

    float x;
    x = internal_frag();

    printf("internal fragmentation: %.4f\n", x);
    printf("external fragmentation: %.4f\n", external_frag());

    sbmem_close();
    sbmem_remove();
}
