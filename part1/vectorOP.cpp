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
  vector<tuple<float *, __pp_vec_float, __pp_vec_int>> destValues;

  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
    float *valuesPtr = values + i;  // trace current pointer location
    float *outputPtr = output + i;
    int *exponentsPtr = exponents + i;
    int maskWidth = i + VECTOR_WIDTH <= N ? VECTOR_WIDTH : N - i;  // decide calculate units

    __pp_vec_float destValue;
    __pp_vec_float outputValue;
    __pp_vec_int exponentsValue;

    __pp_mask dataMask = _pp_init_ones(maskWidth);
    _pp_vload_float(destValue, valuesPtr, dataMask);
    _pp_vload_int(exponentsValue, exponentsPtr, dataMask);  // load *exponents according to dataMask 
    _pp_vset_float(outputValue, 1, dataMask);
    _pp_vstore_float(outputPtr, outputValue, dataMask);  // initialize *output to 1

    tuple<float *, __pp_vec_float, __pp_vec_int> destValueTuple(outputPtr, destValue, exponentsValue);
    destValues.push_back(destValueTuple);
  }

  for (auto &dv : destValues) {
    __pp_mask mask = _pp_init_ones();
    __pp_mask greaterThanZeroMask = _pp_mask_not(mask);
    __pp_vec_int zeroExp = _pp_vset_int(0);
    _pp_vgt_int(greaterThanZeroMask, get<2>(dv), zeroExp, mask);

    while (_pp_cntbits(greaterThanZeroMask))  // while exponents is still a positive number
    {
      __pp_vec_float dvResult;
      __pp_vec_float baseValue;
      __pp_vec_float maxValue = _pp_vset_float(9.999999);
      __pp_vec_int zeroExp = _pp_vset_int(0);
      __pp_vec_int ones;  // as a subtrahend of exponentsValue
      __pp_mask maxValueMask = _pp_mask_not(mask);

      _pp_vload_float(baseValue, get<0>(dv), mask);  // load current output value to baseValue
      _pp_vmult_float(dvResult, baseValue, get<1>(dv), greaterThanZeroMask);  // multiply by it's original value
      _pp_vgt_float(maxValueMask, dvResult, maxValue, mask);  // compare current result to 9.999999
      _pp_vset_float(dvResult, 9.999999, maxValueMask);  // clamp max value
      _pp_vstore_float(get<0>(dv), dvResult, greaterThanZeroMask);

      _pp_vset_int(ones, 1, greaterThanZeroMask);
      _pp_vsub_int(get<2>(dv), get<2>(dv), ones, greaterThanZeroMask);  // update exponents value

      greaterThanZeroMask = _pp_mask_not(mask);
      _pp_vgt_int(greaterThanZeroMask, get<2>(dv), zeroExp, mask);  // update greaterThanZeroMask
    }
  }
}

// returns the sum of all elements in values
// You can assume N is a multiple of VECTOR_WIDTH
// You can assume VECTOR_WIDTH is a power of 2
float arraySumVector(float *values, int N)
{
  float returnValue = 0;
  __pp_vec_float finalResult;
  __pp_mask mask = _pp_init_ones();
  _pp_vset_float(finalResult, 0, mask);

  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
    float *valuesPtr = values + i;  // trace current pointer location
    __pp_vec_float vecValue = _pp_vset_float(0);  // initialize
    __pp_vec_float vecResult = _pp_vset_float(0);

    _pp_vload_float(vecValue, valuesPtr, mask);  // load current "value" to "vecValue"
    _pp_hadd_float(vecValue, vecValue);
    _pp_interleave_float(vecResult, vecValue);
    _pp_vadd_float(finalResult, finalResult , vecResult, mask);
  }

  for (int i = 0; i < VECTOR_WIDTH / 2; i++)
    returnValue += finalResult.value[i];

  return returnValue;
}