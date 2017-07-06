#include "ap_int.h"

// Maximum order of generator polynomial = 10
ap_uint<1> pn_seq_gen_lfsr(ap_uint<1> load, ap_uint<1> pn_req, ap_uint<10> poly, ap_uint<10> seed, ap_uint<4> order){
//#pragma HLS INTERFACE ap_none port=out_bit
#pragma HLS INTERFACE ap_ctrl_none port=return
// For a 63 length (maximal) length sequence, the generator poly is x^6 + x^5 + 1. Let's say the seed is 000001
// Input poly : 0000110000    Input Seed : 0000010000
	ap_uint<1> out_bit, next_bit;
#pragma HLS RESET variable=next_bit
	static ap_uint<10> poly_reg, shift_reg, next_reg_state;
#pragma HLS RESET variable=next_reg_state
#pragma HLS RESET variable=shift_reg
#pragma HLS RESET variable=poly_reg


	next_bit = (shift_reg & poly_reg).xor_reduce();
	next_reg_state = (next_bit, shift_reg(9,1)); // shift all the bits
	out_bit = shift_reg[10 - order];// take output only in the middle of the register (or the LSB for order = 10)

	if(load){
		shift_reg = seed;
	    poly_reg = poly;
	}
	else
        {
		if(pn_req){
			shift_reg = next_reg_state;
		}
    }
	return out_bit;

}
