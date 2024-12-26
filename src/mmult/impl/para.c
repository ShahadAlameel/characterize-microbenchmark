/* para.c
 *
 * Author:
 * Date  :
 *
 *  Description
 */

/* Standard C includes */
#include <stdlib.h>
#include <pthread.h>

/* Include common headers */
#include "common/macros.h"
#include "common/types.h"

/* If we are on Darwin, include the compatibility header */
#if defined(__APPLE__)
#include "common/mach_pthread_compatibility.h"
#endif

/* Include application-specific headers */
#include "include/types.h"

#define CACHE_LINE 64

// Align to prevent false sharing
typedef struct {
    float val;
    char padding[CACHE_LINE - sizeof(float)];
} aligned_float_t;

void* matrix_multiply_parallel(void* args) {
    args_t* parsed_args = (args_t*)args;

    const float* src0 = parsed_args->input1;
    const float* src1 = parsed_args->input2;
    float* dest = parsed_args->output;
    const int b = parsed_args->b;
    const int m = parsed_args->m;
    const int n = parsed_args->n;
    const int p = parsed_args->p;
    const int thread_id = parsed_args->tid;
    const int num_threads = parsed_args->nthreads;

    // Divide work among threads by rows
    const int chunk_size = m / num_threads;
    const int start_row = thread_id * chunk_size;
    const int end_row = (thread_id == num_threads - 1) ? m : (thread_id + 1) * chunk_size;

    aligned_float_t local_val __attribute__((aligned(CACHE_LINE)));

    // Process assigned rows
    for (int ii = start_row; ii < end_row; ii += b) {
        for (int jj = 0; jj < p; jj += b) {
            for (int kk = 0; kk < n; kk += b) {
                for (int i = ii; i < ii + b && i < end_row; i++) {
                    for (int j = jj; j < jj + b && j < p; j++) {
                        local_val.val = dest[i * p + j];
                        #pragma GCC unroll 8
                        for (int k = kk; k < kk + b && k < n; k++) {
                            local_val.val += src0[i * n + k] * src1[k * p + j];
                        }
                        dest[i * p + j] = local_val.val;
                    }
                }
            }
        }
    }
    return NULL;
}

void* impl_parallel(void* args) {
    args_t* parsed_args = (args_t*)args;
    const float* src0 = parsed_args->input1;
    const float* src1 = parsed_args->input2;
    float* dest = parsed_args->output;
    const int b = parsed_args->b;
    const int m = parsed_args->m;
    const int n = parsed_args->n;
    const int p = parsed_args->p;
    const int num_threads = parsed_args->nthreads;

    // Create an array of threads
    pthread_t threads[num_threads];

    // Create an array of thread arguments 
    args_t thread_args[num_threads];

    // Create threads and assign work 
    for (int i = 0; i < num_threads; i++) {
        
            thread_args[i].input1 = src0;
            thread_args[i].input2 = src1;
            thread_args[i].output = dest;
            thread_args[i].b = b;
            thread_args[i].m = m;
            thread_args[i].n = n;
            thread_args[i].p = p;
            thread_args[i].tid = i;
            thread_args[i].nthreads = num_threads; // Pass num_threads to thread arguments
        pthread_create(&threads[i], NULL, matrix_multiply_parallel, &thread_args[i]);
    }

    // Wait for all the threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    return NULL;
}