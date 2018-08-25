#ifndef WV_H264SPS_H
#define WV_H264SPS_H

#include <stdint.h>
#include <stdlib.h>
#include <cstdbool>
#include "wv_h264Bits.h"

typedef struct wv_h264sps
{
	uint32_t  m_profile_idc;
	uint32_t  m_level_idc;
	uint32_t  m_sps_id;

	// for uncommon profile...
	uint32_t   m_chroma_format_idc;
	bool      m_separate_colour_plane_flag;
	uint32_t  m_bit_depth_luma;
	uint32_t  m_bit_depth_chroma;
	bool      m_qpprime_y_zero_transform_bypass_flag;
	bool      m_seq_scaling_matrix_present_flag;
	// ...for uncommon profile

	uint32_t  m_log2_max_frame_num;
	uint32_t  m_poc_type;
	uint32_t  m_log2_max_poc_cnt;
	uint32_t  m_log2_max_num_ref_frames;
	bool      m_gaps_in_frame_num_value_allowed_flag;
	uint32_t  m_pic_width_in_mbs;
	uint32_t  m_pic_height_in_map_units;
	uint32_t  m_pic_height_in_mbs;	// not defined in spec, derived...
	bool      m_frame_mbs_only_flag;
	bool      m_mb_adaptive_frame_field_flag;
	bool      m_direct_8x8_inference_flag;
	bool      m_frame_cropping_flag;
	uint32_t  m_frame_crop_offset[4];
	bool      m_vui_parameters_present_flag;

	uint32_t  m_reserved;

}WV_H264SPS;

int32_t Parse_as_seq_param_set(WV_H264SPS * sps, uint8_t * m_pSODB);

#endif