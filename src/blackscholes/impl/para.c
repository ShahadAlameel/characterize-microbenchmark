/* Standard C includes */
#include <stdlib.h>

#include <string.h>
#include <math.h>
#include <pthread.h>
#include <ctype.h>

/* Include common headers */
#include "common/macros.h"
#include "common/types.h"

/* Include application-specific headers */
#include "include/types.h"
#include <stdio.h>

#define inv_sqrt_2xPI 0.39894228040143270286

// Define the number of threads 
#define num_threads 10


float CNDF_paralell (float InputX) 
{
    int sign;

    float OutputX;
    float xInput;
    float xNPrimeofX;
    float expValues;
    float xK2;
    float xK2_2, xK2_3;
    float xK2_4, xK2_5;
    float xLocal, xLocal_1;
    float xLocal_2, xLocal_3;

    // Check for negative value of InputX
    if (InputX < 0.0) {
        InputX = -InputX;
        sign = 1;
    } else 
        sign = 0;

    xInput = InputX;
 
    // Compute NPrimeX term common to both four & six decimal accuracy calcs
    expValues = exp(-0.5f * InputX * InputX);
    xNPrimeofX = expValues;
    xNPrimeofX = xNPrimeofX * inv_sqrt_2xPI;

    xK2 = 0.2316419 * xInput;
    xK2 = 1.0 + xK2;
    xK2 = 1.0 / xK2;
    xK2_2 = xK2 * xK2;
    xK2_3 = xK2_2 * xK2;
    xK2_4 = xK2_3 * xK2;
    xK2_5 = xK2_4 * xK2;
    
    xLocal_1 = xK2 * 0.319381530;
    xLocal_2 = xK2_2 * (-0.356563782);
    xLocal_3 = xK2_3 * 1.781477937;
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_4 * (-1.821255978);
    xLocal_2 = xLocal_2 + xLocal_3;
    xLocal_3 = xK2_5 * 1.330274429;
    xLocal_2 = xLocal_2 + xLocal_3;

    xLocal_1 = xLocal_2 + xLocal_1;
    xLocal   = xLocal_1 * xNPrimeofX;
    xLocal   = 1.0 - xLocal;

    OutputX  = xLocal;
    
    if (sign) {
        OutputX = 1.0 - OutputX;
    }
    
    return OutputX;
} 


float blackScholes_parallel(float sptprice, float strike, float rate, float volatility,
                   float otime, char otype,float timet )
{
    float OptionPrice;

    // local private working variables for the calculation
    float xStockPrice;
    float xStrikePrice;
    float xRiskFreeRate;
    float xVolatility;
    float xTime;
    float xSqrtTime;

    float logValues;
    float xLogTerm;
    float xD1; 
    float xD2;
    float xPowerTerm;
    float xDen;
    float d1;
    float d2;
    float FutureValueX;
    float NofXd1;
    float NofXd2;
    float NegNofXd1;
    float NegNofXd2;    
    int type_int = ( tolower ( otype ) == 'p')? 1 : 0;

    xStockPrice = sptprice;
    xStrikePrice = strike;
    xRiskFreeRate = rate;
    xVolatility = volatility;

    xTime = otime;
    xSqrtTime = sqrt(xTime);

    logValues = log(sptprice / strike);
        
    xLogTerm = logValues;
        
    
    xPowerTerm = xVolatility * xVolatility;
    xPowerTerm = xPowerTerm * 0.5;
        
    xD1 = xRiskFreeRate + xPowerTerm;
    xD1 = xD1 * xTime;
    xD1 = xD1 + xLogTerm;

    xDen = xVolatility * xSqrtTime;
    xD1 = xD1 / xDen;
    xD2 = xD1 - xDen;

    d1 = xD1;
    d2 = xD2;
    
    NofXd1 = CNDF_paralell(d1);
    NofXd2 = CNDF_paralell(d2);

    FutureValueX = strike * (exp(-(rate)*(otime)));        
    if (type_int == 0) {            
        OptionPrice = (sptprice * NofXd1) - (FutureValueX * NofXd2);
    } else { 
        NegNofXd1 = (1.0 - NofXd1);
        NegNofXd2 = (1.0 - NofXd2);
        OptionPrice = (FutureValueX * NegNofXd2) - (sptprice * NegNofXd1);
    }
    
    return OptionPrice;
}

void* compute_BlackScholes(void* args) {
   args_t* thread_args = (args_t*)args;

  // Allocate memory for local output
  float* local_output = malloc(thread_args->num_stocks * sizeof(float));

  // Processing the work for the thread
  for (size_t i = 0; i < thread_args->num_stocks; ++i) {
          local_output[i] = blackScholes_parallel(
            thread_args->sptPrice[i], 
            thread_args->strike[i], 
            thread_args->rate[i], 
            thread_args->volatility[i], 
            thread_args->otime[i], 
            thread_args->otype[i],
            0);
      }
  memcpy(thread_args->output, local_output, thread_args->num_stocks * sizeof(float));
  free(local_output);
  return NULL;
}

void* impl_parallel(void* args) {
  args_t* para_args = (args_t*)args;

  size_t num_stocks = para_args->num_stocks;
  float* sptPrice = para_args->sptPrice;
  float* strike = para_args->strike;
  float* rate = para_args->rate;
  float* volatility = para_args->volatility;
  float* otime = para_args->otime;
  char* otype = para_args->otype;
  float* output = para_args->output;

  // Calculate the chunk size for each thread
  size_t chunk_size = num_stocks / num_threads;

  // Create an array of threads
  pthread_t threads[num_threads];

  // Create an array of thread arguments 
  args_t thread_args[num_threads];

  // Create threads and assign work 
  for (int i = 0; i < num_threads; i++) {
    // Assign number of stocks for each thread (handling the remaining num_stocks)
    thread_args[i].num_stocks = (i == num_threads - 1) ? (num_stocks - i * chunk_size) : chunk_size;

    thread_args[i].sptPrice = &sptPrice[i * chunk_size];
    thread_args[i].strike = &strike[i * chunk_size];
    thread_args[i].rate = &rate[i * chunk_size];
    thread_args[i].volatility = &volatility[i * chunk_size];
    thread_args[i].otime = &otime[i * chunk_size];
    thread_args[i].otype = &otype[i * chunk_size];
    thread_args[i].output = &output[i * chunk_size];

    // Create the thread
    pthread_create(&threads[i], NULL, compute_BlackScholes, (void*)&thread_args[i]);
  }

  // Wait for all the threads to finish
   for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  // Writing the results is only for testing purposes, must be commented when evaluating the implementation
  FILE* file = fopen("parallelized_output.csv", "w");
  fprintf(file, "StockID,OptionPrice\n");
  for (size_t i = 0; i < num_stocks; i++) {
      fprintf(file, "%zu,%.6f\n", i, output[i]);
  }
  fclose(file);
  return NULL;
}
