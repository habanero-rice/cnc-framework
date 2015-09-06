#ifndef _CNCOCR_CHOLESKY_COMMON_H_
#define _CNCOCR_CHOLESKY_COMMON_H_

#if CNCOCR_TG
struct timeval { u64 t; };
#else
#include <sys/time.h>
#endif

#endif /*_CNCOCR_CHOLESKY_COMMON_H_*/
