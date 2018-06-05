/*===-- MemoryProfiling.c - Support library for memory profiling --------------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source      
|* License. See LICENSE.TXT for details.                                      
|* 
|*===----------------------------------------------------------------------===*|
|* 
|* This file implements the call back routines for the conflicts profiling
|* instrumentation pass.  This should be used with the -conflicts LLVM pass.
|*
\*===----------------------------------------------------------------------===*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

typedef unsigned long long count_t;

static unsigned count_num;
static count_t *counts;

static void output_end(void) {
  //fprintf(stderr, "output_end()\n");
    fprintf(stderr, "Conflicts statistics:\n");
    for (unsigned i = 0; i < count_num; ++i) {
        fprintf(stderr, "\tconflicts at %u: %llu\n", i, counts[i]);
    }
    free(counts);
}

void llvm_start_conflict_profiling(unsigned id_num) {
  //fprintf(stderr, "llvm_start_conflict_profiling()\n");
    count_num = id_num;
    counts = (count_t *)calloc(count_num, sizeof(count_t));
    atexit(output_end);
}

void llvm_conflict_profiling(void *addr1, void *addr2, unsigned id) {
  //fprintf(stderr, "llvm_conflict_profiling()\n");
    fprintf(stderr, "%p\t%p\t%u\n", addr1, addr2, id);
    ++counts[id];
}
