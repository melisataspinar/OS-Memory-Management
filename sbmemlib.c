#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdbool.h>

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
#define SHM_NAME "/shm_name_shared_segment"
#define SEMNAME_MUTEX "/name_sem_mutex"

// Define semaphore(s)
sem_t *sem_mutex; 

// Define your stuctures and variables. 
void *shm_start;  

struct alloc_info {
    int req_size;
    int size;
    int base;
    bool is_split;
    bool is_allocated;
    bool is_garbage;
};

struct alloc_info *alloc_infos;

struct sbmem_info {
    int seg_size;
    int num_pids;
    int pids[10];
    int M; // number of bytes of info part
    int N; // number of nodes in the tree
};

struct sbmem_info *s_info;

/** Helper functions **/

bool isPowerOfTwo(ulong x)
{
    return (x & (x - 1)) == 0;
}

int info_bytes(int segmentsize) {
    int res = 0;
    res += sizeof(struct sbmem_info);
    res += (2*(segmentsize/128)-1) * sizeof(struct alloc_info);
    return res;
}

void init_alloc_info(struct alloc_info *a_info, int size, int base) {
    a_info->is_garbage = false;
    a_info->is_allocated = false;
    a_info->is_split = false;
    a_info->size = size;
    a_info->base = base;
}

void print_sinfo(struct sbmem_info *sinfo) {
    printf("printing sinfo at address %lu:\n", (unsigned long int ) sinfo);
    printf("seg_size: %d, num_pids: %d, N %d, M %d\n", 
        sinfo->seg_size, sinfo->num_pids, sinfo->N, sinfo->M);
}

void print_ainfo(struct alloc_info *ainfo) {
    printf("printing ainfo:\n");
    printf("size: %d, base: %d, is_allocated %d, is_garbage %d, is_split %d\n", 
        ainfo->size, ainfo->base, ainfo->is_allocated, ainfo->is_garbage, ainfo->is_split);
}

void print_tree() {
    int i;
    int p = 1;
    int k = 0;
    for (i = 0; i < s_info->N; i++) {
        struct alloc_info ainfo = alloc_infos[i];

        if (!ainfo.is_garbage) {
            printf("%d,%d,%d ", ainfo.size, ainfo.is_split, ainfo.is_allocated);
        }

        k++;
        if (k == p) {
            k = 0;
            p *=2;
            printf("\n");
        }
    }
}

float internal_frag() {
    int i;
    int total_alloc = 0;
    int frag_alloc = 0;
    for (i = 0; i < s_info->N; i++) {
        if (alloc_infos[i].is_allocated) {
            total_alloc += alloc_infos[i].size;
            frag_alloc += (alloc_infos[i].size - alloc_infos[i].req_size);
        }
    }
    return ((float) frag_alloc) / ((float) total_alloc);
}

float external_frag() {
    int i;
    int total_alloc = s_info->seg_size;
    int frag_alloc = 0;
    for (i = 0; i < s_info->N; i++) {
        if (!alloc_infos[i].is_allocated && !alloc_infos[i].is_split && !alloc_infos[i].is_garbage) {
            if (alloc_infos[i].size < 4096) {
                frag_alloc += alloc_infos[i].size;
            }
        }
    }
    return ((float) frag_alloc) / ((float) total_alloc);
}

/** Official Functions **/

int sbmem_init(int segmentsize)
{
    struct stat sbuf; 
    int fd;  

    if (segmentsize < 32768  || segmentsize > 262144 || !isPowerOfTwo((ulong) segmentsize)) {
        printf("segmentsize (%d) invalid, init failed...\n", segmentsize);
        return -1;
    }
    sem_unlink(SEMNAME_MUTEX);
    sem_mutex = sem_open(SEMNAME_MUTEX, O_RDWR | O_CREAT, 0660, 1);
    shm_unlink(SHM_NAME);

    fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660);

    int M = info_bytes(segmentsize);
    ftruncate(fd, M + segmentsize);    /* set size of shared memmory */
    fstat(fd, &sbuf);
    shm_start = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, 
			 MAP_SHARED, fd, 0);

	close(fd); /* no longer need the descriptor */

    // initialize sbmem_info part
    s_info = (struct sbmem_info *) shm_start; 
    s_info->M = M;
    s_info->N = (2*(segmentsize/128)-1);
    s_info->seg_size = segmentsize;
    s_info->num_pids = 0;

    // initialize alloc infos

    alloc_infos = (struct alloc_info *) &(s_info[1]);
    init_alloc_info(alloc_infos, segmentsize, 0);

    int i;
    struct alloc_info *curr;
    for (i = 1; i < s_info->N; i++) {
        curr = &(alloc_infos[i]);
        curr->is_garbage = true;
    }

    return 0; 
}


int sbmem_remove()
{
    shm_unlink(SHM_NAME);

    // TODO: unlink semaphores
    sem_unlink(SEMNAME_MUTEX);
    return (0); 
}


int sbmem_open()
{
    struct stat sbuf; 
    int fd; 
    sem_mutex = sem_open(SEMNAME_MUTEX, O_RDWR);

    // TODO: synchronize
    sem_wait(sem_mutex);

    // get the sbmem_info
    fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660);
    fstat(fd, &sbuf);
    s_info = (struct sbmem_info *) mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, 
			 MAP_SHARED, fd, 0);
    
    
    // check if process already opened sbmem
    int i;
    for (i = 0; i < s_info->num_pids; i++) {
        if (s_info->pids[i] == getpid()) {
            return 0;
        }
    }

    // if not, check if there is space available
    if (s_info->num_pids >= 10) {
        return -1;
    }

    fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660);
    fstat(fd, &sbuf);
    shm_start = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, 
			 MAP_SHARED, fd, 0);

    s_info = (struct sbmem_info *) shm_start; 
    alloc_infos = (struct alloc_info *) &(s_info[1]);

    // if there is space, add the process pid to tracked pids
    s_info->pids[s_info->num_pids] = getpid();
    s_info->num_pids++;
    sem_post(sem_mutex);

    return 0; 
}


void *sbmem_alloc (int reqsize)
{
    sem_wait(sem_mutex);
    // invalid size

    // TODO: make 128
    if (reqsize < 128 || reqsize > 4096) {
        sem_post(sem_mutex);
        return NULL;
    }

    // TODO: syncronize

    // find the first best fitting segment
    int i;
    int best_i = -1;
    struct alloc_info *a_info;
    for (i = 0; i < s_info->N; i++) {
        a_info = &(alloc_infos[i]);
        if (!a_info->is_garbage && !a_info->is_allocated && !a_info->is_split && a_info->size >= reqsize) {
            if (best_i == -1) {
                best_i = i;
            } else {
                if (a_info->size < alloc_infos[best_i].size) {
                    best_i = i;
                }
            }
        }
    }

    // alloc failed
    if (best_i == -1) {
        sem_post(sem_mutex);
        return NULL;
    }

    // split if necessary
    a_info = &(alloc_infos[best_i]);

    while (a_info->size >= reqsize * 2) {
        a_info->is_split = true;
        // left child
        init_alloc_info(&(alloc_infos[2*best_i+1]), (a_info->size)/2, a_info->base);
        // right child
        init_alloc_info(&(alloc_infos[2*best_i+2]), (a_info->size)/2, a_info->base + a_info->size/2);

        // set to left child
        a_info = &(alloc_infos[2*best_i+1]);
        best_i = 2*best_i+1;
    }
    // minimal split acquired
    a_info->is_allocated = true;
    a_info->req_size = reqsize;

    sem_post(sem_mutex);

    return shm_start + s_info->M + (unsigned long int) a_info->base;
}


void sbmem_free (void *p)
{
    sem_wait(sem_mutex);
    
    // TODO: synchronize

    // find base of allocation
    int base = (p - shm_start - s_info->M);
    
    // find index of allocation
    
    int i = 0;
    struct alloc_info *a_info = &(alloc_infos[i]);
    while (a_info->base != base) {
        int diff = base - a_info->base;
        if (diff < a_info->size/2) {
            a_info = &(alloc_infos[2*i + 1]);
            i = 2*i + 1;
        } else {
            a_info = &(alloc_infos[2*i + 2]);
            i = 2*i + 2;
        }
    }

    while (a_info->is_split) {
        i = 2*i+1;
        a_info = &alloc_infos[i];
    }

    a_info->is_allocated = false;

    // check for possible merges
    struct alloc_info *buddy;
    while (true) {
        // root has no buddy
        if (i == 0) {
            break;
        }
        
        // find buddy
        if (i % 2 == 0) {
            buddy = &(alloc_infos[i - 1]);
        } else {
            buddy = &(alloc_infos[i + 1]);
        }

        // break if buddy is not free
        if (buddy->is_split || buddy->is_allocated) {
            break;
        }

        // merge if buddy is free
        buddy->is_garbage = true;
        a_info->is_garbage = true;

        // set to parent
        a_info = &(alloc_infos[i / 2]);
        a_info->is_split = false;
        i /= 2;
    }
    sem_post(sem_mutex);
}


int sbmem_close()
{
    // TODO: synchronize
    sem_wait(sem_mutex);

    int i;
    for (i = 0; i < s_info->num_pids; i++) {
        if (s_info->pids[i] == getpid()) {
            s_info->num_pids--;
            int j;
            for (j = i; j < s_info->num_pids; j++) {
                s_info->pids[j] = s_info->pids[j+1];
            }
        }
    }
    sem_post(sem_mutex);
    sem_close(sem_mutex);
    
    // TODO: munmap?

    return (0); 
}
