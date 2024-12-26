/* naive.c
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

/* Naive Implementation */
//#pragma GCC push_options
//#pragma GCC optimize ("O1")
void* impl_scalar_naive(void* args)
{

  args_t* parsed_args = (args_t*)args;

  register float* 	   dest = (float*)(parsed_args->output);
  register const float* src1 = (const float*)(parsed_args->input1);
  register const float* src2 = (const float*)(parsed_args->input2);
  //register size_t 	size = parsed_args->size/4;
  register size_t	m    = parsed_args->m;
  register size_t	p    = parsed_args->p;
  register size_t	n    = parsed_args->n;

  for(register int i = 0; i < m; i++) {
    for(register int j = 0; j < n; j++) {
      dest[i*n + j] = 0;                      
      for(register int k = 0; k < p; k++) {
          dest[i*n + j] += src1[i*p + k] * src2[k*n + j];  
      }
    }
  }

    return NULL;

}
//#pragma GCC pop_options
