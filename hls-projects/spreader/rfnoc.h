#include "ap_int.h"

 struct rfnoc_axis{
     ap_int<32>       data;
     ap_uint<1>       last;
   };
