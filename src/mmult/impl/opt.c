/* opt.c
 *
 * Author: Elaf
 * Date  : 23/11/2024
 *
 *  Description
 */

/* Standard C includes */
#include <stdlib.h>

/* Include common headers */
#include "common/macros.h"
#include "common/types.h"

/* Include application-specific headers */
#include "include/types.h"

/* Alternative Implementation */
//#pragma GCC push_options
//#pragma GCC optimize ("O1")


static inline int min(int a, int b) { //try without static inline
    return (a < b) ? a : b;
}

void* impl_scalar_opt(void* args)
{

  args_t* parsed_args = (args_t*)args;   

  register const float *src0 = (const float *)(parsed_args->input1);
  register const float *src1 = (const float *)(parsed_args->input2);
  register float *dest = (float *)(parsed_args->output);

  register int b = parsed_args->b;
  register int m = parsed_args->m;
  register int n = parsed_args->n;
  register int p = parsed_args->p;


  float val = 0.0;
  for (register int ii = 0; ii < m; ii += b) {
      for (register int jj = 0; jj < p; jj += b) {
          for (register int kk = 0; kk < n; kk += b) {
              for (register int i = ii; i < ii + b && i < m; i++) {
                  for (register int j = jj; j < jj + b && j < p; j++) {
                      val = dest[i * p + j];
                      for (register int k = kk; k < kk + b && k < n; k++) {
                          val += src0[i * n + k] * src1[k * p + j];
                      }
                      dest[i * p + j] = val;
                  }
              }
          }
      }
  }
  

  return NULL;
}
//#pragma GCC pop_options