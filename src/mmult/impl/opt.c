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

  register float*       dest = (float*)(parsed_args->output);
  register const float* src1 = (const float*)(parsed_args->input1);
  register const float* src2 = (const float*)(parsed_args->input2);

  register size_t       m    = parsed_args->m;
  register size_t       p    = parsed_args->p;
  register size_t       n    = parsed_args->n;
  register size_t 	    b    = parsed_args->b;
  register float 	      val;


    for (int i = 0; i < m; i += b) {
      for (int j = 0; j < n; j += b) {
          for (int x = i; x < i + b && x < m; x++) {
              for (int y = j; y < j + b && y < n; y++) {
                  dest[x * n + y] = 0.0f;
              }
          }
      }
  }
  
  for (int ii = 0; ii < m; ii += b) {
    for (int jj = 0; jj < n; jj += b) {
      for (int kk = 0; kk < p; kk += b) {
        for (int i = ii; i < min(ii + b, m); i++) {
            for (int j = jj; j < min(jj + b, n); j++) {
                val = dest[i * n + j];
                for (int k = kk; k < min(kk + b, p); k++) {
                    val += src1[i * p + k] * src2[k * n + j];
                }
                dest[i * n + j] = val;
          }
        }
      }
    }
  }
  

  return NULL;
}
//#pragma GCC pop_options