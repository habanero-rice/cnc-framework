#ifndef _CNCOCR_CHOLESKY_TYPES_H_
#define _CNCOCR_CHOLESKY_TYPES_H_

#include "Cholesky_common.h"

typedef struct CholeskyArguments {
    // Path to input matrix file
    char inFile[256];
} CholeskyArgs;

#define CHOLESKY_USE_FILE 1

#endif /*_CNCOCR_CHOLESKY_TYPES_H_*/
