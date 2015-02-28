#ifndef __cplusplus
#   error This must be compiled as C++
#endif
#include <new.h>

/* this is a C++ decorated function, make an extern C wrapper */

extern "C" _PNH
dm_set_new_handler(_PNH pnh)
{
   return _set_new_handler(pnh);
}
