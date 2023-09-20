In this project we implement a memory management library called sbmem.a (buddy memory allocation from shared memory). 

Processes use our library to allocate memory space dynamically, instead of using the well-known malloc function.

A shared memory object, i.e., a shared memory segment, is created first. Memory space allocations are made from that segment to the requesting processes. Our library implements the necessary initialization, allocation and deallocation routines. It keeps track of the allocated and free spaces in the memory segment, for which it uses the Buddy memory allocation algorithm.

Makefile is included.
