/* vec.c
 *
 * Author:
 * Date  :
 *
 *  Description
 */

/* Standard C includes */
#include <stdlib.h>
#include <stdio.h>

/* Include common headers */
#include "common/macros.h"
#include "common/types.h"

/* Include application-specific headers */
#include "include/types.h"
#include <immintrin.h>

/* Alternative Implementation */
#pragma GCC push_options
#pragma GCC optimize ("O1")

void* save_result_opt(float* dest, int m, int n) {
  FILE* file = fopen("mmult_opt_output.csv", "w");
  if (file == NULL) {
    perror("Error opening file");
  }

  fprintf(file, "[");
  for (size_t i = 0; i < m; i++) {
      for (size_t j = 0; j < n; j++) {
          fprintf(file, ",%.6f", dest[i * m + j]);
      }
      if (i < m - 1) {
          fprintf(file, ",\n");
      }
  }
  fprintf(file, "]");
  fclose(file);

  return NULL;
}

__m256 simd_mmult(const float* src1, const float* src2, float* dest,
                  __m256i vm, int m, int p, int n) {

  __m256 res  = _mm256_setzero_ps();

  __m256 vec1, vec2, vresult;

  for(register int i = 0; i < m; i++) {
    for(register int j = 0; j < n; j++) {  
      for(register int k = 0; k < p; k++) {
        vec1 = _mm256_set1_ps(src1[i*p + k]);
        vec2 = _mm256_maskload_ps(&src2[k * n], vm);
        res  = _mm256_maskload_ps(&dest[i * n], vm);
        res  = _mm256_add_ps(_mm256_mul_ps(vec1, vec2), res);
        _mm256_maskstore_ps(&dest[i * n], vm, res);
      }
    }
  }

  return res;
}

void* impl_vector(void* args)
{
  #if defined(__amd64__) || defined(__x86_64__)
  /* Get the argument struct */
  args_t* parsed_args = (args_t*)args;

  /* Get all the arguments */
  register       float*   dest = (      float*)(parsed_args->output); // mxn
  register       float* destcp = (      float*)(parsed_args->output); //copy of pointer
  register const float*   src0 = (const float*)(parsed_args->input1); // mxp
  register const float*   src1 = (const float*)(parsed_args->input2); // pxn
  register       size_t size =                  parsed_args->size;    // sizeof(float) * m * n

  register size_t       m    = parsed_args->m;
  register size_t       p    = parsed_args->p;
  register size_t       n    = parsed_args->n;

  __m256i vm         = _mm256_set1_epi32(0x80000000);
  const int max_vlen = 32 / sizeof(float); // eight vectors

  for (register size_t hw_vlen, i = 0; i < size; i += hw_vlen) {

    register int rem = size - i;
    hw_vlen = rem < max_vlen ? rem : max_vlen;        /* num of elems      */
    if (hw_vlen < max_vlen) {
      unsigned int m[max_vlen];
      for (size_t j = 0; j < max_vlen; j++)
        m[j] = (j < hw_vlen) ? 0x80000000 : 0x00000000;
      vm = _mm256_setr_epi32(m[0], m[1], m[2], m[3],
                             m[4], m[5], m[6], m[7]);
    }

    __m256 res =simd_mmult(src0,src1, dest, vm, m,  p,  n);            /* Store output      */

    src0 += hw_vlen;                                  /* -\                */
    src1 += hw_vlen;                                  /*   |-> ptr arith   */
    dest += hw_vlen;                                  /* -/                */
  }

  //writing the results is only for testing purposes, must be commented when evaluating the implementation
  //save_result_opt(destcp,m,n);

  /* Done */
  return NULL;
#elif defined(__aarch__) || defined(__aarch64__) || defined(__arm64__)
#endif

}
#pragma GCC pop_options
