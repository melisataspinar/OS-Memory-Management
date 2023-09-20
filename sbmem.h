

void print_tree();

float internal_frag();
float external_frag();

int sbmem_init(int segmentsize); 
int sbmem_remove(); 

int sbmem_open(); 
void *sbmem_alloc (int size);
void sbmem_free(void *p);
int sbmem_close(); 



    
    
