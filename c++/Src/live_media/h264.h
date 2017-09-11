#pragma once

struct h264_slice_t
{
	int		i_slice_type;
	int		i_frame_num;
	int		i_pic_order_cnt_lsb;
};

struct AVRational_t{
	int num; ///< numerator
	int den; ///< denominator
};

struct h264_sps_t{

	int profile_idc;
	int level_idc;
	int transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
	int log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
	int poc_type;                      ///< pic_order_cnt_type
	int log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
	int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int offset_for_top_to_bottom_field;
	int poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
	int ref_frame_count;               ///< num_ref_frames
	int gaps_in_frame_num_allowed_flag;
	int mb_width;                      ///< frame_width_in_mbs_minus1 + 1
	int mb_height;                     ///< frame_height_in_mbs_minus1 + 1
	int frame_mbs_only_flag;
	int mb_aff;                        ///<mb_adaptive_frame_field_flag
	int direct_8x8_inference_flag;
	int crop;                   ///< frame_cropping_flag
	int crop_left;              ///< frame_cropping_rect_left_offset
	int crop_right;             ///< frame_cropping_rect_right_offset
	int crop_top;               ///< frame_cropping_rect_top_offset
	int crop_bottom;            ///< frame_cropping_rect_bottom_offset
	int vui_parameters_present_flag;
	AVRational_t sar;
	int timing_info_present_flag;
	int num_units_in_tick;
	int time_scale;
	int fixed_frame_rate_flag;
	short offset_for_ref_frame[256]; //FIXME dyn aloc?
	int bitstream_restriction_flag;
	int num_reorder_frames;
	int scaling_matrix_present;
	unsigned char scaling_matrix4[6][16];
	unsigned char scaling_matrix8[2][64];
};

void h264_decode_annexb(unsigned char* dst, int* dstlen, const unsigned char* src, const int srclen);
void h264_get_nal_type(int* p_nal_type, const unsigned char* p_nal);
void h264_find_frame_end(bool* p_found_frame_start, bool* p_new_frame, const unsigned char* p_nal, const int nal_length, int i_nal_type);

void h264_decode_slice(h264_slice_t* p_slice, unsigned char* p_nal,  int n_nal_size, int i_nal_type, const h264_sps_t* p_sps);
bool h264_decode_seq_parameter_set(unsigned char* p_nal,  int n_nal_size, h264_sps_t* p_sps);