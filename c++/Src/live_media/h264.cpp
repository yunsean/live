#include "h264.h"

void h264_decode_annexb(unsigned char* dst, int* dstlen, const unsigned char* src, const int srclen )
{
	unsigned char *dst_sav = dst;
	const unsigned char *end = &src[srclen];

	while (src < end)
	{
		if (src < end - 3 && src[0] == 0x00 && src[1] == 0x00 &&
			src[2] == 0x03)
		{
			*dst++ = 0x00;
			*dst++ = 0x00;

			src += 3;
			continue;
		}
		*dst++ = *src++;
	}

	*dstlen = (int)(dst - dst_sav);
}

void h264_get_nal_type(int* p_nal_type, const unsigned char *p_nal)
{
	int i_nal_hdr;

	i_nal_hdr = p_nal[3];
	*p_nal_type = i_nal_hdr & 0x1f;
}

void h264_find_frame_end(bool* p_found_frame_start, bool* p_new_frame, const unsigned char* p_nal, const int nal_length, int i_nal_type)
{
	if (i_nal_type == 1/*NAL_SLICE*/ || i_nal_type == 2/*NAL_DPA*/ || i_nal_type == 5/*NAL_IDR_SLICE*/)
		*p_found_frame_start = true;

	if (i_nal_type == 7/*NAL_SPS*/ || i_nal_type == 8/*NAL_PPS*/ || i_nal_type == 9/*NAL_AUD*/)
		*p_found_frame_start = false;

	if( (*p_found_frame_start) )
	{
		for (int i = 3; i < nal_length; i++)
		{
			if (p_nal[i] & 0x80)
			{
				*p_found_frame_start = false;
				*p_new_frame = true;
				return;
			}
		}
	}

	*p_new_frame = false;
}
