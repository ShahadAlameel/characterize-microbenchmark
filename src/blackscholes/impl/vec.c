/* vec.c
 *
 * Author: Elaf
 * Date  : 24/12/2024
 *
 *  Description
 *  SIMD Implementation of Black-Scholes.
 */

/* Standard C includes */
#include <stdlib.h>

/* Include common headers */
#include "common/macros.h"
#include "common/types.h"
#include "common/vmath.h" //for exp function

#include <ctype.h>
#include <string.h>

#include <math.h> 
/* Include application-specific headers */
#include "include/types.h"
#include <stdio.h>
#include <immintrin.h>



#define inv_sqrt_2xPI 0.39894228040143270286

__m256 CNDF_vec(__m256 InputX) 
{
    // Check for negative value of InputX
    //isolating the sign bit
    __m256 sign_bit = _mm256_set1_ps(-0.0f);
    __m256 sign     = _mm256_and_ps(InputX, sign_bit);
    //set the sign bits of InputX to zero
    //https://stackoverflow.com/questions/23847377/how-does-this-function-compute-the-absolute-value-of-a-float-through-a-not-and-a
    InputX         = _mm256_andnot_ps(sign_bit, InputX);
    

    __m256 one      = _mm256_set1_ps(1.0f);
 
    //Compute NPrimeX term common to both four & six decimal accuracy calcs
    __m256 expValues = _mm256_mul_ps(_mm256_set1_ps(-0.5f),
                                     _mm256_mul_ps(InputX, InputX)
                                     );
    expValues        = _mm256_exp_ps(expValues);
    __m256 xNPrimeofX = _mm256_mul_ps(expValues, _mm256_set1_ps(inv_sqrt_2xPI));

    __m256 xK2      = _mm256_mul_ps(_mm256_set1_ps(0.2316419), InputX);
    xK2             = _mm256_add_ps(one, xK2);
    xK2             = _mm256_div_ps(one, xK2);
    __m256 xK2_2    = _mm256_mul_ps(xK2,   xK2);
    __m256 xK2_3    = _mm256_mul_ps(xK2_2, xK2);
    __m256 xK2_4    = _mm256_mul_ps(xK2_3, xK2);
    __m256 xK2_5    = _mm256_mul_ps(xK2_4, xK2);
    
    __m256 xLocal_1 = _mm256_mul_ps(xK2,   _mm256_set1_ps(0.319381530));
    __m256 xLocal_2 = _mm256_mul_ps(xK2_2, _mm256_set1_ps((-0.356563782)));
    __m256 xLocal_3 = _mm256_mul_ps(xK2_3, _mm256_set1_ps(1.781477937));
    xLocal_2        = _mm256_add_ps(xLocal_2, xLocal_3);
    xLocal_3        = _mm256_mul_ps(xK2_4, _mm256_set1_ps(-1.821255978));
    xLocal_2        = _mm256_add_ps(xLocal_2, xLocal_3);
    xLocal_3        = _mm256_mul_ps(xK2_5, _mm256_set1_ps(1.330274429));
    xLocal_2        = _mm256_add_ps(xLocal_2, xLocal_3);
    xLocal_1        = _mm256_add_ps(xLocal_2, xLocal_1);
    __m256 xLocal   = _mm256_mul_ps(xLocal_1, xNPrimeofX);
    xLocal          = _mm256_sub_ps(one, xLocal);
    __m256 OutputX  = xLocal;
    
    __m256 temp = _mm256_sub_ps(one, OutputX); // 1-outputX
    OutputX     = _mm256_blendv_ps(OutputX, temp, sign); //only subtract 1 from the corresponding negative input values in the mask sign
  
    return OutputX;
} 


__m256 blackScholes_vec(__m256 sptprice, __m256 strike, __m256 rate, __m256 volatility, __m256 otime, __m256 otype, int timet)
{
    __m256 xStockPrice = sptprice;
    __m256 xStrikePrice = strike;
    __m256 xRiskFreeRate = rate;
    __m256 xVolatility = volatility;
    __m256 one = _mm256_set1_ps(1.0f);

    __m256 xTime = otime;
    __m256 xSqrtTime = _mm256_sqrt_ps(xTime);

    __m256 logValues = _mm256_log_ps(_mm256_div_ps(sptprice, strike));
        
    __m256 xLogTerm = logValues;
        
    
   __m256 xPowerTerm = _mm256_mul_ps(xVolatility, xVolatility);
   xPowerTerm = _mm256_mul_ps(xPowerTerm, _mm256_set1_ps(0.5f));
        
    __m256 xD1 = _mm256_add_ps(xRiskFreeRate, xPowerTerm);
    xD1 = _mm256_mul_ps(xD1, xTime);
    xD1 = _mm256_add_ps(xD1, xLogTerm);

    __m256 xDen = _mm256_mul_ps(xVolatility, xSqrtTime);
    xD1 = _mm256_div_ps(xD1, xDen);
    __m256 xD2 = _mm256_sub_ps(xD1, xDen);

    __m256 d1 = xD1;
    __m256 d2 = xD2;
    
    __m256 NofXd1 = CNDF_vec(d1);
    __m256 NofXd2 = CNDF_vec(d2);

    __m256 negative_rate = _mm256_mul_ps(_mm256_set1_ps(-1.0f),rate); 
    __m256 FutureValueX = _mm256_mul_ps(strike, _mm256_exp_ps(
                                            _mm256_mul_ps(negative_rate,otime))
                                        );     

    __m256 NegNofXd1 = _mm256_sub_ps(one, NofXd1);
    __m256 NegNofXd2 = _mm256_sub_ps(one, NofXd2);

    __m256 ifTrue  = _mm256_sub_ps(_mm256_mul_ps(sptprice, NofXd1),
                                  _mm256_mul_ps(FutureValueX, NofXd2));

    __m256 ifFalse = _mm256_sub_ps(_mm256_mul_ps(FutureValueX, NegNofXd2),
                                  _mm256_mul_ps(sptprice, NegNofXd1));


    __m256 OptionPrice = _mm256_blendv_ps(ifTrue,ifFalse,otype);

    // __m256 OptionPrice = _mm256_sub_ps( 
    //     _mm256_mul_ps(sptprice, NofXd1) ,
    //     _mm256_blendv_ps(_mm256_mul_ps(FutureValueX, NofXd2), 
    //                     _mm256_mul_ps(FutureValueX, NegNofXd2),
    //                     _mm256_mul_ps(sptprice, NegNofXd1)
    //                     )
    //     );

    return OptionPrice;
}
/* Alternative Implementation */
void* impl_vector(void* args)
{
  #if defined(__amd64__) || defined(__x86_64__)
  args_t* arguments  = (args_t*)args;
  size_t num_stocks  = arguments->num_stocks;
  float* sptPrice    = arguments->sptPrice;
  float* strike      = arguments->strike;
  float* rate        = arguments->rate;
  float* volatility  = arguments->volatility;
  float* otime       = arguments->otime;
  char*  otype       = arguments->otype;
  float* dest        = arguments->output;
  float* dest_cp     = arguments->output;

  __m256i vm         = _mm256_set1_epi32(0x80000000);
  const int max_vlen = 32 / sizeof(float); // eight vectors

  for (register size_t hw_vlen, i = 0; i < num_stocks; i += hw_vlen) {

    register int rem = num_stocks - i;
    hw_vlen = rem < max_vlen ? rem : max_vlen;        /* num of elems      */
    if (hw_vlen < max_vlen) {
      unsigned int m[max_vlen];
      for (size_t j = 0; j < max_vlen; j++)
        m[j] = (j < hw_vlen) ? 0x80000000 : 0x00000000;
      vm = _mm256_setr_epi32(m[0], m[1], m[2], m[3],
                          m[4], m[5], m[6], m[7]);
    }

    __m256 v_sptPrice       = _mm256_maskload_ps(sptPrice,      vm);   /* Load vectors from */
    __m256 v_strike         = _mm256_maskload_ps(strike,        vm);   /* src0 and src1     */
    __m256 v_rate           = _mm256_maskload_ps(rate,          vm);
    __m256 v_volatility     = _mm256_maskload_ps(volatility,    vm);
    __m256 v_otime          = _mm256_maskload_ps(otime,         vm);

    float vec_otype[8];
    for (int j = 0; j < 8; j++) {
        vec_otype[j] = ( tolower ( otype[j] ) == 'p')? -0.0f : 0.0f;
    }
    __m256 v_otype          = _mm256_maskload_ps(vec_otype, vm);

    __m256 res  = blackScholes_vec(v_sptPrice, v_strike, v_rate, v_volatility, v_otime, v_otype, 0);      /* Do the compute    */

    _mm256_maskstore_ps(dest, vm, res);                     /* Store output      */

    sptPrice   += hw_vlen;                                  /* -\                */
    strike     += hw_vlen;                                  /*   |               */
    rate       += hw_vlen;                                  /*   |               */
    volatility += hw_vlen;                                  /*   |-> ptr arith   */
    otime      += hw_vlen;                                  /*   |               */
    otype      += hw_vlen;                                  /*   |               */
    dest       += hw_vlen;                                  /* -/                */
  }

  // writing the results is only for testing purposes, must be commented when evaluating the implementation
  FILE* file = fopen("vec_output.csv", "w");
  fprintf(file, "StockID,OptionPrice\n");
  for (size_t i = 0; i < num_stocks; i++) {
      fprintf(file, "%zu,%.6f\n", i, dest_cp[i]);
  }
  fclose(file);

return NULL;  
#elif defined(__aarch__) || defined(__aarch64__) || defined(__arm64__)
#endif
}
