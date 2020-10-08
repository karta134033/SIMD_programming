#include "PPintrin.h"
#include <tuple>

// implementation of absSerial(), but it is vectorized using PP intrinsics
void absVector(float *values, float *output, int N)
{
  __pp_vec_float x;
  __pp_vec_float result;
  __pp_vec_float zero = _pp_vset_float(0.f);
  __pp_mask maskAll, maskIsNegative, maskIsNotNegative;

  //  Note: Take a careful look at this loop indexing.  This example
  //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
  //  Why is that the case?
  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {

    // All ones
    maskAll = _pp_init_ones();

    // All zeros
    maskIsNegative = _pp_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _pp_vload_float(x, values + i, maskAll); // x = values[i];

    // Set mask according to predicate
    _pp_vlt_float(maskIsNegative, x, zero, maskAll); // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _pp_vsub_float(result, zero, x, maskIsNegative); //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _pp_mask_not(maskIsNegative); // } else {

    // Execute instruction ("else" clause)
    _pp_vload_float(result, values + i, maskIsNotNegative); //   output[i] = x; }

    // Write results back to memory
    _pp_vstore_float(output + i, result, maskAll);
  }
}

void clampedExpVector(float *values, int *exponents, float *output, int N)
{
  //
  // PP STUDENTS TODO: Implement your vectorized version of
  // clampedExpSerial() here.
  //
  // Your solution should work for any value of
  // N and VECTOR_WIDTH, not just when VECTOR_WIDTH divides N
  //
  
  vector<tuple<float *, __pp_vec_float, int *, __pp_mask>> destValues;
  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
    float *valuesPtr = values + i;
    float *outputPtr = output + i;

     __pp_vec_float destValue;
    int maskWidth = i + VECTOR_WIDTH <= N ? VECTOR_WIDTH : N - i;  // decide calculate units
    __pp_mask mask = _pp_init_ones(maskWidth);
    _pp_vload_float(destValue, valuesPtr, mask);

    int *exponentsCopy = new int[VECTOR_WIDTH];
    int exponentsEnd = i + VECTOR_WIDTH;
    for (int j = i, k = 0; j < exponentsEnd; j++, k++)
    {  // copy the origin exponents without changing it and initial output value
      exponentsCopy[k] = exponents[j];
      if (mask.value[k])
        output[j] = 1;
      else
        output[j] = 0;
    }

    tuple<float *, __pp_vec_float, int *, __pp_mask> destValueTuple(outputPtr, destValue, exponentsCopy, mask);
    destValues.push_back(destValueTuple);
  }

  for (auto dv : destValues)
  {
    int maxExponents = 0;
    for (int j = 0; j < VECTOR_WIDTH; j++)
      if (get<2>(dv)[j] > maxExponents) maxExponents = get<2>(dv)[j];

    while (maxExponents >= 1)
    {
      __pp_vec_float dvResult;
      __pp_vec_float baseValue;
      _pp_vload_float(baseValue, get<0>(dv), get<3>(dv));
      _pp_vmult_float(dvResult, baseValue, get<1>(dv), get<3>(dv));
      for (int j = 0; j < VECTOR_WIDTH; j++)
      {
        if (!get<2>(dv)[j] && get<3>(dv).value[j])  // when exponents is 0 and not be calculated
        {
          get<0>(dv)[j] = 1;
          get<3>(dv).value[j] = 0;
        }
        else if (get<3>(dv).value[j])
        {
          get<0>(dv)[j] = dvResult.value[j] > 9.999999 ? 9.999999 : dvResult.value[j];  // assign value to output
          get<2>(dv)[j]--;
          if (!get<2>(dv)[j] && get<3>(dv).value[j])
            get<3>(dv).value[j] = 0;
        }
      }
      maxExponents--;
    }
  }
}

// returns the sum of all elements in values
// You can assume N is a multiple of VECTOR_WIDTH
// You can assume VECTOR_WIDTH is a power of 2
float arraySumVector(float *values, int N)
{

  //
  // PP STUDENTS TODO: Implement your vectorized version of arraySumSerial here
  //

  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
  }

  return 0.0;
}