#include "h264.h"
#include "vlc_bits.h"

#ifndef UNUSED 
#define UNUSED(x)	x = (x)
#endif

void h264_decode_slice(h264_slice_t* p_slice, unsigned char* p_nal,  int n_nal_size, int i_nal_type, const h264_sps_t* p_sps)
{
	bs_t s;
	bs_init(&s, p_nal, n_nal_size);

	bs_read_ue( &s );	// first_mb_in_slice
	p_slice->i_slice_type = bs_read_ue( &s );	// slice type

	int i_pic_parameter_set_id = bs_read_ue( &s );
	p_slice->i_frame_num = bs_read( &s, p_sps->log2_max_frame_num + 4 );

	int i_field_pic_flag = 0;
	int i_bottom_field_flag = -1;
	if( !p_sps->frame_mbs_only_flag)
	{
		/* field_pic_flag */
		i_field_pic_flag = bs_read( &s, 1 );
		if( i_field_pic_flag )
			i_bottom_field_flag = bs_read( &s, 1 );
	}

	int i_idr_pic_id;
	if( i_nal_type == 5/*NAL_SLICE_IDR*/ )
		i_idr_pic_id = bs_read_ue( &s );

	int i_delta_pic_order_cnt_bottom = -1;
	int i_delta_pic_order_cnt0 = 0;
	int i_delta_pic_order_cnt1 = 0;
	
	p_slice->i_pic_order_cnt_lsb = 0;

	if( p_sps->poc_type == 0 )
	{
		p_slice->i_pic_order_cnt_lsb = bs_read( &s, p_sps->log2_max_poc_lsb + 4 );
		//if( g_pic_order_present_flag && !i_field_pic_flag )
		//	i_delta_pic_order_cnt_bottom = bs_read_se( &s );
	}
	else if( (p_sps->poc_type == 1) &&
		(!p_sps->delta_pic_order_always_zero_flag) )
	{
		i_delta_pic_order_cnt0 = bs_read_se( &s );
		//if( g_pic_order_present_flag && !i_field_pic_flag )
		//	i_delta_pic_order_cnt1 = bs_read_se( &s );
	}

	UNUSED(i_pic_parameter_set_id);
	UNUSED(i_bottom_field_flag);
	UNUSED(i_idr_pic_id);
	UNUSED(i_delta_pic_order_cnt_bottom);
	UNUSED(i_delta_pic_order_cnt0);
	UNUSED(i_delta_pic_order_cnt1);
}
