#include <hls_stream.h>
#include "ap_int.h"
#include "rfnoc.h"

void averaging(hls::stream<rfnoc_axis> i_data, hls::stream<rfnoc_axis> o_data, ap_uint<32> threshold, ap_uint<10> seq_len, ap_uint<3> log_avg_size)
{
#pragma HLS PIPELINE II=1
#pragma HLS INTERFACE axis port=o_data
#pragma HLS INTERFACE axis port=i_data
#pragma HLS INTERFACE ap_ctrl_none port=return

	static ap_uint<32> data_in_reg;
	static ap_uint<1> data_in_valid;

	static ap_uint<10> seq_len_reg;
	static ap_uint<8> avg_size_reg;
	static ap_uint<3> log_avg_size_reg;

	enum averagingState {ST_IDLE = 0, ST_FIRST_BLK, ST_NOT_FIRST_BLK};
	static averagingState currentState;
#pragma HLS RESET variable=currentState

	static hls::stream<ap_uint<40> > data_fifo;
#pragma HLS STREAM variable=data_fifo depth=1024 dim=1
#pragma HLS RESOURCE variable=data_fifo core=FIFO_BRAM
 // 40 bits to accommodate for accumulation over 128 samples (max averaging size)
    static hls::stream<ap_uint<32> > out_fifo;
#pragma HLS STREAM variable=out_fifo depth=1024 dim=1
#pragma HLS RESOURCE variable=out_fifo core=FIFO_BRAM

	static ap_uint<40> pfetch_data;
#pragma HLS DEPENDENCE variable=pfetch_data inter false
	static ap_uint<10> wr_cnt;
#pragma HLS RESET variable=wr_cnt
	static ap_uint<10> rd_cnt;
#pragma HLS RESET variable=rd_cnt
    static ap_int<1> threshold_met;
#pragma HLS RESET variable=threshold_met

	static ap_uint<8> blk_cnt;
	static ap_uint<9> out_sample_cnt;
	rfnoc_axis out_sample;
	ap_uint<32> tmp_data;

	switch(currentState){
	   case ST_IDLE :
		   if(data_in_valid) 
		   {
                 if(!threshold_met)
                 {
                      seq_len_reg = seq_len;
                      log_avg_size_reg = log_avg_size;
                      avg_size_reg = 1 << log_avg_size;
                 }
                 if(!threshold_met && (data_in_reg > threshold))
                      threshold_met = 1;
                 if((!threshold_met && (data_in_reg > threshold)) || threshold_met)
                 {
			          data_fifo.write(data_in_reg);
			          wr_cnt = wr_cnt + 1;
			          currentState = ST_FIRST_BLK;
                 }
		   }
		   break;
	   case ST_FIRST_BLK:
		   if(data_in_valid)
		   {
			   data_fifo.write(data_in_reg);
			   if(wr_cnt == seq_len_reg - 1)
			   {
				   wr_cnt = 0;
				   //data_fifo.read(pfetch_data);
				   //rd_ptr = rd_ptr + 1;
				   blk_cnt = 1;
				   currentState = ST_NOT_FIRST_BLK;
			   }
			   else
			   {
				   wr_cnt = wr_cnt + 1;
				   currentState = ST_FIRST_BLK;
			   }
		   }
		   break;
	   case ST_NOT_FIRST_BLK:
		   if(data_in_valid)
		   {
               if(blk_cnt < (avg_size_reg - 1))
               {
                     data_fifo.write(data_in_reg + data_fifo.read());
			         if(wr_cnt == seq_len_reg - 1)
			         {
				        wr_cnt = 0;
				        blk_cnt = blk_cnt + 1;
				        currentState = ST_NOT_FIRST_BLK;
			         }
			         else
			         {
				        wr_cnt = wr_cnt + 1;
			         }
               }
               else
               {
                      tmp_data = ((data_in_reg + data_fifo.read())).range(31+log_avg_size_reg, log_avg_size_reg);
            	      out_fifo.write(tmp_data);
                      if(wr_cnt == seq_len_reg - 1)
                      {
                         wr_cnt = 0;
                         blk_cnt = 0;
                         currentState = ST_IDLE;
                      }
                      else
                      {
                         wr_cnt = wr_cnt + 1;
                      }
               }
		   }
		   break;
        }


        if(!out_fifo.empty())
        {	   
	   out_sample.data = (out_fifo.read());

	   if ((out_sample_cnt == 259) || (rd_cnt == seq_len_reg - 1) )
           {
	       out_sample.last = 1;
	       out_sample_cnt = 0;
	   }
           else
	   {
	       out_sample.last = 0;
	       out_sample_cnt = out_sample_cnt + 1;
	   }

           if(rd_cnt == seq_len_reg - 1)
              rd_cnt = 0;
           else
              rd_cnt = rd_cnt + 1;


	   o_data.write(out_sample);
        }	


	if(!i_data.empty())
	{
		data_in_reg = i_data.read().data;
		data_in_valid = 1;
	}
	else
		data_in_valid = 0;


}
